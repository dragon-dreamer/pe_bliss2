#include "pe_bliss2/security/image_hash.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include "buffers/input_buffer_interface.h"
#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/detail/image_optional_header.h"
#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/image/checksum.h"
#include "pe_bliss2/image/image.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include "cryptopp/md5.h"
#include "cryptopp/sha.h"

#include "utilities/math.h"
#include "utilities/safe_uint.h"

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
		case unable_to_read_image_data:
			return "Unable to read image data";
		case unsupported_hash_algorithm:
			return "Unsupported hash algorithm";
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
void update_hash(buffers::input_buffer_interface& buf, std::size_t from, std::size_t to,
	CryptoPP::HashTransformation& hash)
{
	if (from > to)
		throw pe_error(hash_calculator_errc::unable_to_read_image_data);

	try
	{
		auto physical_size = to - from;
		if (const auto* data = buf.get_raw_data(from, physical_size); data)
		{
			hash.Update(reinterpret_cast<const CryptoPP::byte*>(data), physical_size);
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
			hash.Update(reinterpret_cast<const CryptoPP::byte*>(temp.data()), read_bytes);
		}
	}
	catch (const pe_error&)
	{
		std::throw_with_nested(pe_error(hash_calculator_errc::unable_to_read_image_data));
	}
}

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

void calculate_hash_impl(const image::image& instance,
	CryptoPP::HashTransformation& hash, std::vector<std::byte>& result)
{
	result.resize(hash.DigestSize());

	const auto checksum_offset = image::get_checksum_offset(instance);
	const auto cert_table_entry_offset = get_cert_table_entry_offset(instance);

	auto headers_buffer = instance.get_full_headers_buffer().data();
	if (headers_buffer->physical_size() < cert_table_entry_offset
		+ core::data_directories::directory_packed_size
		|| cert_table_entry_offset < checksum_offset)
	{
		throw pe_error(hash_calculator_errc::invalid_security_directory_offset);
	}

	update_hash(*headers_buffer, 0u, checksum_offset, hash);
	update_hash(*headers_buffer,
		checksum_offset + sizeof(image::image_checksum_type),
		cert_table_entry_offset, hash);
	update_hash(*headers_buffer,
		cert_table_entry_offset + core::data_directories::directory_packed_size,
		headers_buffer->physical_size(), hash);

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
		update_hash(*section_buf, 0u, section_buf->physical_size(), hash);
	}

	std::size_t remaining_size = instance.get_overlay().physical_size();

	if (!instance.get_data_directories().has_security())
		throw pe_error(hash_calculator_errc::unable_to_read_image_data);

	const auto security_dir_size = instance.get_data_directories()
		.get_directory(core::data_directories::directory_type::security)->size;
	if (security_dir_size > remaining_size)
		throw pe_error(hash_calculator_errc::unable_to_read_image_data);

	remaining_size -= security_dir_size;
	if (remaining_size)
	{
		auto overlay_buf = instance.get_overlay().data();
		update_hash(*overlay_buf, 0u, remaining_size, hash);
	}

	hash.Final(reinterpret_cast<CryptoPP::byte*>(result.data()));
}

template<typename Hash>
void calculate_hash_impl(const image::image& instance, std::vector<std::byte>& result)
{
	Hash hash;
	calculate_hash_impl(instance, hash, result);
}
} //namespace

std::vector<std::byte> calculate_hash(authenticode_digest_algorithm algorithm,
	const pe_bliss::image::image& instance)
{
	std::vector<std::byte> result;

	switch (algorithm)
	{
	case authenticode_digest_algorithm::md5:
		calculate_hash_impl<CryptoPP::Weak::MD5>(instance, result);
		break;
	case authenticode_digest_algorithm::sha1:
		calculate_hash_impl<CryptoPP::SHA1>(instance, result);
		break;
	case authenticode_digest_algorithm::sha256:
		calculate_hash_impl<CryptoPP::SHA256>(instance, result);
		break;
	default:
		throw pe_error(hash_calculator_errc::unsupported_hash_algorithm);
	}

	return result;
}

template<typename RangeType>
bool is_hash_valid(const authenticode_pkcs7<RangeType>& pkcs7,
	const pe_bliss::image::image& instance)
{
	return std::ranges::equal(
		calculate_hash(pkcs7.get_digest_algorithm(), instance),
		pkcs7.get_image_hash());
}

template bool is_hash_valid<authenticode_span_range_type>(
	const authenticode_pkcs7<authenticode_span_range_type>& pkcs7,
	const pe_bliss::image::image& instance);
template bool is_hash_valid<authenticode_vector_range_type>(
	const authenticode_pkcs7<authenticode_vector_range_type>& pkcs7,
	const pe_bliss::image::image& instance);

} //namespace pe_bliss::security
