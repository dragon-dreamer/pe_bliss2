#include "pe_bliss2/security/image_hash.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "buffers/input_buffer_interface.h"
#include "buffers/input_buffer_stateful_wrapper.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "cryptopp/md5.h"
#include "cryptopp/sha.h"

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/detail/image_optional_header.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/image/checksum.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/security/buffer_hash.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/hash_helpers.h"

#include "utilities/math.h"
#include "utilities/safe_uint.h"
#include "utilities/variant_helpers.h"

namespace
{
struct hash_calculator_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "hash_calculator";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::hash_calculator_errc;
		switch (static_cast<pe_bliss::security::hash_calculator_errc>(ev))
		{
		case invalid_security_directory_offset:
			return "Invalid security directory offset";
		case invalid_section_data:
			return "Invalid section data";
		case invalid_section_alignment:
			return "Invalid section alignment";
		case too_big_page_hash_buffer:
			return "Too big page hash buffer";
		default:
			return {};
		}
	}
};

const hash_calculator_error_category hash_calculator_error_category_instance;
} //namespace

namespace pe_bliss::security
{

std::error_code make_error_code(hash_calculator_errc e) noexcept
{
	return { static_cast<int>(e), hash_calculator_error_category_instance };
}

namespace
{
std::uint32_t get_cert_table_entry_offset(const image::image& instance)
{
	utilities::safe_uint cert_table_entry_offset
		= instance.get_dos_header().get_descriptor()->e_lfanew;
	try
	{
		cert_table_entry_offset += core::file_header::descriptor_type::packed_size;
		cert_table_entry_offset += core::image_signature::descriptor_type::packed_size;
		cert_table_entry_offset += instance.get_optional_header().get_size_of_structure();
		cert_table_entry_offset += core::data_directories::directory_packed_size
			* static_cast<std::uint32_t>(core::data_directories::directory_type::security);
	}
	catch (const std::system_error&)
	{
		throw pe_error(hash_calculator_errc::invalid_security_directory_offset);
	}

	return cert_table_entry_offset.value();
}

void try_init_page_hash_state(
	const image::image& instance,
	const page_hash_options& options,
	CryptoPP::HashTransformation& page_hash,
	image_hash_result& result,
	std::optional<page_hash_state>& state)
{
	const auto section_alignment = instance
		.get_optional_header().get_raw_section_alignment();
	if (!std::has_single_bit(section_alignment) || section_alignment == 1u
		|| section_alignment > options.max_section_alignment)
	{
		result.page_hash_errc = hash_calculator_errc::invalid_section_alignment;
		return;
	}

	std::size_t page_count = 1u + (instance.get_full_headers_buffer().physical_size()
		+ section_alignment - 1u) / section_alignment;
	for (const auto& data : instance.get_section_data_list())
	{
		page_count += (data.physical_size() + section_alignment - 1u)
			/ section_alignment;
	}

	const std::size_t single_page_hash_size = page_hash.DigestSize() + sizeof(std::uint32_t);
	const std::size_t total_page_hashes_size = page_count * single_page_hash_size;
	if (total_page_hashes_size > options.max_page_hashes_size
		|| total_page_hashes_size / page_count != single_page_hash_size)
	{
		result.page_hash_errc = hash_calculator_errc::too_big_page_hash_buffer;
		return;
	}

	state.emplace(page_hash, section_alignment)
		.reserve(total_page_hashes_size);
}

void calculate_hash_impl(const image::image& instance,
	CryptoPP::HashTransformation& hash, image_hash_result& result,
	const page_hash_options* page_hashes,
	CryptoPP::HashTransformation* page_hash)
{
	std::optional<page_hash_state> state;
	if (page_hash && page_hashes)
	{
		try_init_page_hash_state(instance,
			*page_hashes, *page_hash, result, state);
	}

	const auto checksum_offset = image::get_checksum_offset(instance);
	const auto cert_table_entry_offset = get_cert_table_entry_offset(instance);

	auto headers_buffer = instance.get_full_headers_buffer().data();
	if (headers_buffer->physical_size() < cert_table_entry_offset
		+ core::data_directories::directory_packed_size
		|| cert_table_entry_offset < checksum_offset)
	{
		throw pe_error(hash_calculator_errc::invalid_security_directory_offset);
	}

	update_hash(*headers_buffer, 0u, checksum_offset, hash, state);
	if (state)
		state->add_skipped_bytes(sizeof(std::uint32_t)); //sizeof checksum

	update_hash(*headers_buffer,
		checksum_offset + sizeof(image::image_checksum_type),
		cert_table_entry_offset, hash, state);
	if (state)
		state->add_skipped_bytes(core::data_directories::directory_packed_size);

	update_hash(*headers_buffer,
		cert_table_entry_offset + core::data_directories::directory_packed_size,
		headers_buffer->physical_size(), hash, state);

	if (state)
		state->next_page();

	using section_ref = std::pair<
		std::reference_wrapper<const section::section_header>,
		std::reference_wrapper<const section::section_data>>;
	std::vector<section_ref> sections_to_hash;
	const auto& section_headers = instance.get_section_table().get_section_headers();
	const auto& section_data = instance.get_section_data_list();
	if (section_data.size() != section_headers.size())
		throw pe_error(hash_calculator_errc::invalid_section_data);

	for (std::size_t i = 0; i != section_headers.size(); ++i)
	{
		if (!section_headers[i].get_descriptor()->size_of_raw_data)
			continue;

		sections_to_hash.emplace_back(section_headers[i], section_data[i]);
	}

	std::sort(sections_to_hash.begin(), sections_to_hash.end(),
		[](const section_ref& ref_l, const section_ref& ref_r) {
		return ref_l.first.get().get_pointer_to_raw_data()
			< ref_r.first.get().get_pointer_to_raw_data();
	});

	for (const auto& sect : sections_to_hash)
	{
		auto section_buf = sect.second.get().data();
		update_hash(*section_buf, 0u, section_buf->physical_size(), hash, state);
		if (state)
			state->next_page();
	}

	std::size_t remaining_size = instance.get_overlay().physical_size();

	if (!instance.get_data_directories().has_security())
		throw pe_error(hash_helpers_errc::unable_to_read_data);

	const auto security_dir_size = instance.get_data_directories()
		.get_directory(core::data_directories::directory_type::security)->size;
	if (security_dir_size > remaining_size)
		throw pe_error(hash_helpers_errc::unable_to_read_data);

	remaining_size -= security_dir_size;
	if (remaining_size)
	{
		auto overlay_buf = instance.get_overlay().data();
		update_hash(*overlay_buf, 0u, remaining_size, hash);
	}

	result.image_hash.resize(hash.DigestSize());
	hash.Final(reinterpret_cast<CryptoPP::byte*>(result.image_hash.data()));

	if (page_hashes && state)
		result.page_hashes = std::move(*state).get_page_hashes();
}

using hash_variant_type = std::variant<std::monostate, CryptoPP::Weak::MD5, CryptoPP::SHA1, CryptoPP::SHA256>;
void init_hash(hash_variant_type& hash, digest_algorithm algorithm)
{
	switch (algorithm)
	{
	case digest_algorithm::md5:
		hash.emplace<CryptoPP::Weak::MD5>();
		break;
	case digest_algorithm::sha1:
		hash.emplace<CryptoPP::SHA1>();
		break;
	case digest_algorithm::sha256:
		hash.emplace<CryptoPP::SHA256>();
		break;
	default:
		throw pe_error(buffer_hash_errc::unsupported_hash_algorithm);
	}
}

CryptoPP::HashTransformation* get_hash(hash_variant_type& hash)
{
	return std::visit(utilities::overloaded{
		[](auto& obj) -> CryptoPP::HashTransformation* { return &obj; },
		[](std::monostate) -> CryptoPP::HashTransformation* { return nullptr; }
	}, hash);
}
} //namespace

image_hash_result calculate_hash(digest_algorithm algorithm,
	const pe_bliss::image::image& instance,
	const page_hash_options* page_hash_opts)
{
	image_hash_result result;

	hash_variant_type image_hash;
	hash_variant_type page_hash;
	init_hash(image_hash, algorithm);
	if (page_hash_opts)
	{
		try
		{
			init_hash(page_hash, page_hash_opts->algorithm);
		}
		catch (const pe_error& e)
		{
			result.page_hash_errc = e.code();
			page_hash_opts = nullptr;
		}
	}

	calculate_hash_impl(instance, *get_hash(image_hash),
		result, page_hash_opts, get_hash(page_hash));
	return result;
}

} //namespace pe_bliss::security
