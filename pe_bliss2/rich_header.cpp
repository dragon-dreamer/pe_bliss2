#include "pe_bliss2/rich_header.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <climits>
#include <cstring>
#include <iterator>
#include <system_error>
#include <tuple>

#include "pe_bliss2/detail/image_dos_header.h"
#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{

struct rich_header_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "image_signature";
	}

	std::string message(int ev) const override
	{
		switch (static_cast<pe_bliss::rich_header_errc>(ev))
		{
		case pe_bliss::rich_header_errc::invalid_rich_header_offset:
			return "Provided DOS stub does not correspond to Rich header";
		default:
			return {};
		}
	}
};

const rich_header_error_category rich_header_error_category_instance;

static constexpr std::array rich_signature{
	std::byte{'h'}, std::byte{'c'}, std::byte{'i'}, std::byte{'R'}
};

} //namespace

namespace pe_bliss
{

std::error_code make_error_code(rich_header_errc e) noexcept
{
	return { static_cast<int>(e), rich_header_error_category_instance };
}

std::size_t rich_header::get_compid_offset() const noexcept
{
	return utilities::math::align_up(dans_offset_ + sizeof(std::uint32_t), compid_alignment);
}

std::size_t rich_header::decode_checksum(const std::vector<std::byte>& data) noexcept
{
	auto rich_it = std::search(data.crbegin(), data.crend(),
		rich_signature.cbegin(), rich_signature.cend());
	if (rich_it == data.crend())
		return data.size();

	auto rich_signature_end_offset = std::distance(data.cbegin(), rich_it.base());
	if (rich_signature_end_offset + sizeof(std::uint32_t) > data.size()
		|| (rich_signature_end_offset % sizeof(std::uint32_t)) != 0)
	{
		return data.size();
	}

	auto rich_end_pointer = data.data() + rich_signature_end_offset;
	detail::packed_serialization<>::deserialize(checksum_, rich_end_pointer);
	return rich_signature_end_offset;
}

bool rich_header::decode_dans_signature(const std::vector<std::byte>& data,
	const std::byte* compid_pointer) noexcept
{
	std::uint32_t dword{};
	while (compid_pointer >= data.data() + sizeof(std::uint32_t))
	{
		compid_pointer -= sizeof(std::uint32_t);
		detail::packed_serialization<>::deserialize(dword, compid_pointer);
		if ((dword ^ checksum_) == dans_signature)
		{
			dans_offset_ = compid_pointer - data.data();
			return true;
		}
	}
	return false;
}

bool rich_header::decode_compids(const std::vector<std::byte>& data,
	const std::byte* rich_signature_pointer)
{
	auto compid_pointer = data.data() + get_compid_offset();
	if ((rich_signature_pointer - compid_pointer) % rich_compid_size)
		return false;

	rich_compid compid{};
	while (compid_pointer + rich_compid_size <= rich_signature_pointer)
	{
		compid_pointer = detail::packed_serialization<>::deserialize(compid, compid_pointer);
		compid.use_count ^= checksum_;
		compid.prod_id ^= checksum_ >> (CHAR_BIT * sizeof(compid.build_number));
		compid.build_number ^= checksum_;
		compids_.emplace_back(compid);
	}
	return true;
}

bool rich_header::deserialize(const dos_stub::dos_stub_data_type& stub_data)
{
	compids_.clear();
	dans_offset_ = 0;

	if (stub_data.empty())
		return false;

	auto rich_signature_end_offset = decode_checksum(stub_data);
	if (rich_signature_end_offset == stub_data.size())
		return false;

	auto rich_end_pointer = stub_data.data() + rich_signature_end_offset;
	auto rich_signature_pointer = rich_end_pointer - sizeof(std::uint32_t);
	if (!decode_dans_signature(stub_data, rich_signature_pointer))
		return false;

	return decode_compids(stub_data, rich_signature_pointer);
}

std::uint32_t rich_header::calculate_checksum(const dos_stub::dos_stub_data_type& stub_data,
	const dos_header& header, std::size_t rich_header_offset) const
{
	if (stub_data.size() < rich_header_offset)
		throw pe_error(rich_header_errc::invalid_rich_header_offset);

	const auto dos_header_data = header.base_struct().serialize();

	static constexpr auto dos_header_data_size = std::tuple_size_v<decltype(dos_header_data)>;
	static_assert((dos_header_data_size % sizeof(std::uint32_t)) == 0);
	static constexpr auto e_lfanew_offset = detail::packed_serialization<>
		::get_field_offset<&detail::image_dos_header::e_lfanew>();

	std::uint32_t checksum = static_cast<std::uint32_t>(dos_header_data_size + rich_header_offset);
	assert((checksum % 4) == 0);

	auto dos_header_data_pointer = dos_header_data.data();
	std::size_t index = 0;
	for (; index != dos_header_data_size; ++index)
	{
		//Skip e_lfanew field
		if (index >= e_lfanew_offset && index < e_lfanew_offset + sizeof(detail::image_dos_header::e_lfanew))
			continue;
		checksum += std::rotl(std::to_integer<std::uint32_t>(
			dos_header_data_pointer[index]), static_cast<uint8_t>(index));
	}

	dos_header_data_pointer = stub_data.data();
	for (std::size_t dans_index = index + rich_header_offset; index != dans_index;
		++index, ++dos_header_data_pointer)
	{
		checksum += std::rotl(std::to_integer<std::uint32_t>(
			*dos_header_data_pointer), static_cast<uint8_t>(index));
	}

	for (const auto& compid : compids_)
	{
		checksum += std::rotl(
			static_cast<std::uint32_t>(compid.build_number)
				| (static_cast<std::uint32_t>(compid.prod_id) << 16u),
			static_cast<std::uint8_t>(compid.use_count));
	}

	return checksum;
}

std::size_t rich_header::calculate_size(std::size_t rich_header_offset) const
{
	std::size_t aligned_offset = rich_header_offset;
	if (!utilities::math::align_up_if_safe(aligned_offset, compid_alignment))
		throw pe_error(rich_header_errc::invalid_rich_header_offset);

	utilities::safe_uint size(aligned_offset - rich_header_offset);
	size += sizeof(dans_signature);
	size += sizeof(std::uint32_t) * 3; //Padding
	size += compids_.size() * rich_compid_size;
	size += rich_signature.size();
	size += sizeof(checksum_);
	if (!utilities::math::is_sum_safe(size.value(), rich_header_offset))
		throw pe_error(rich_header_errc::invalid_rich_header_offset);

	return size.value();
}

std::byte* rich_header::serialize(const dos_header& header,
	dos_stub::dos_stub_data_type& stub_data,
	const rich_header_serialization_options& options)
{
	if (!is_valid())
		return nullptr;

	auto size = calculate_size(options.dos_stub_offset);
	if (stub_data.size() < options.dos_stub_offset + size)
	{
		if (options.allow_dos_stub_buffer_extension)
			stub_data.resize(options.dos_stub_offset + size);
		else
			throw pe_error(rich_header_errc::invalid_rich_header_offset);
	}

	auto aligned_buffer_offset = utilities::math::align_up(
		options.dos_stub_offset, compid_alignment);
	std::uint32_t checksum = checksum_;
	if (options.recalculate_checksum)
		checksum = calculate_checksum(stub_data, header, aligned_buffer_offset);

	std::byte* buffer = stub_data.data() + aligned_buffer_offset;
	buffer = detail::packed_serialization<>::serialize(dans_signature ^ checksum, buffer);
	for (std::size_t i = 0; i != 3; ++i) //padding
		buffer = detail::packed_serialization<>::serialize(checksum, buffer);

	for (auto compid : compids_)
	{
		compid.use_count ^= checksum;
		compid.prod_id ^= checksum >> (CHAR_BIT * sizeof(compid.build_number));
		compid.build_number ^= checksum;
		buffer = detail::packed_serialization<>::serialize(compid, buffer);
	}

	std::copy(rich_signature.crbegin(), rich_signature.crend(), buffer);
	buffer += rich_signature.size();
	buffer = detail::packed_serialization<>::serialize(checksum, buffer);

	assert(static_cast<std::size_t>(buffer - (stub_data.data() + options.dos_stub_offset)) == size);
	return buffer;
}

} //namespace pe_bliss
