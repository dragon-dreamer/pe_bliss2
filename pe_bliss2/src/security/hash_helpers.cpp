#include "pe_bliss2/security/hash_helpers.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <exception>
#include <string>
#include <system_error>
#include <utility>

#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/pe_error.h"

namespace
{
struct hash_helpers_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "hash_helpers";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::hash_helpers_errc;
		switch (static_cast<pe_bliss::security::hash_helpers_errc>(ev))
		{
		case unable_to_read_data:
			return "Unable to read image data";
		default:
			return {};
		}
	}
};

const hash_helpers_error_category hash_helpers_error_category_instance;
} //namespace

namespace pe_bliss::security
{

namespace
{

template<typename UpdateFunc>
void update_hash_impl(buffers::input_buffer_interface& buf, std::size_t from, std::size_t to,
	UpdateFunc&& update_func)
{
	if (from > to)
		throw pe_error(hash_helpers_errc::unable_to_read_data);

	try
	{
		auto absolute_offset = buf.absolute_offset() + from;
		auto physical_size = to - from;
		if (const auto* data = buf.get_raw_data(from, physical_size); data)
		{
			update_func(reinterpret_cast<const CryptoPP::byte*>(data),
				physical_size, absolute_offset);
			return;
		}

		buffers::input_buffer_stateful_wrapper_ref ref(buf);
		ref.set_rpos(from);
		static constexpr std::size_t temp_buffer_size = 512u;
		std::array<std::byte, temp_buffer_size> temp;
		while (physical_size)
		{
			auto read_bytes = std::min(physical_size, temp_buffer_size);
			physical_size -= read_bytes;
			ref.read(read_bytes, temp.data());
			update_func(reinterpret_cast<const CryptoPP::byte*>(temp.data()),
				read_bytes, absolute_offset);
			absolute_offset += read_bytes;
		}
	}
	catch (const pe_error&)
	{
		std::throw_with_nested(pe_error(hash_helpers_errc::unable_to_read_data));
	}
}

} //namespace

std::error_code make_error_code(hash_helpers_errc e) noexcept
{
	return { static_cast<int>(e), hash_helpers_error_category_instance };
}

void update_hash(buffers::input_buffer_interface& buf, std::size_t from, std::size_t to,
	CryptoPP::HashTransformation& hash)
{
	update_hash_impl(buf, from, to, [&hash](const CryptoPP::byte* data, std::size_t size,
		std::size_t /* offset */) {
		hash.Update(data, size);
	});
}

void update_hash(buffers::input_buffer_interface& buf, std::size_t from, std::size_t to,
	page_hash_state& state)
{
	update_hash_impl(buf, from, to, [&state](
		const CryptoPP::byte* data, std::size_t size, std::size_t offset) {
		state.update(data, offset, size);
	});
}

void update_hash(buffers::input_buffer_interface& buf, std::size_t from, std::size_t to,
	CryptoPP::HashTransformation& hash, std::optional<page_hash_state>& state)
{
	update_hash_impl(buf, from, to, [&state, &hash](
		const CryptoPP::byte* data, std::size_t size, std::size_t offset) {
		hash.Update(data, size);
		if (state)
			state->update(data, offset, size);
	});
}

page_hash_state::page_hash_state(CryptoPP::HashTransformation& hash,
	std::size_t page_size)
	: hash_(hash)
	, page_size_(page_size)
{
}

void page_hash_state::update(const CryptoPP::byte* data,
	std::size_t offset, std::size_t size)
{
	last_offset_ = (std::max)(last_offset_, offset + size);

	if (!current_size_)
		next_page_offset_ = offset;

	std::size_t data_offset = 0u;
	while (size)
	{
		const auto remaining_bytes = page_size_ - current_size_;
		if (size < remaining_bytes)
		{
			hash_.Update(data + data_offset, size);
			current_size_ += size;
			return;
		}

		hash_.Update(data + data_offset, remaining_bytes);
		current_size_ += remaining_bytes;
		offset += remaining_bytes;
		data_offset += remaining_bytes;
		next_page();
		size -= remaining_bytes;
		next_page_offset_ = offset;
	}
}

std::byte* page_hash_state::add_blank_page(std::size_t offset)
{
	const auto pos = page_hashes_.size();
	page_hashes_.resize(page_hashes_.size()
		+ hash_.DigestSize() + sizeof(std::uint32_t));
	return detail::packed_serialization<>::serialize<std::uint32_t>(
		static_cast<std::uint32_t>(offset),
		page_hashes_.data() + pos);
}

void page_hash_state::add_skipped_bytes(std::size_t skipped_bytes) noexcept
{
	skipped_bytes_ += skipped_bytes;
}

void page_hash_state::next_page()
{
	static constexpr std::array<CryptoPP::byte, 512> empty_space{};
	if (current_size_)
	{
		const auto page_size = page_size_ - skipped_bytes_;
		while (current_size_ < page_size)
		{
			const auto update_size = (std::min)(page_size - current_size_,
				empty_space.size());
			hash_.Update(empty_space.data(), update_size);
			current_size_ += update_size;
		}
		
		hash_.Final(reinterpret_cast<CryptoPP::byte*>(
			add_blank_page(next_page_offset_)));
		current_size_ = 0u;
	}
	skipped_bytes_ = 0u;
}

std::vector<std::byte> page_hash_state::get_page_hashes() && noexcept
{
	next_page();
	add_blank_page(last_offset_);
	return std::move(page_hashes_);
}

void page_hash_state::reserve(std::size_t size)
{
	page_hashes_.reserve(size);
}

} //namespace pe_bliss::security
