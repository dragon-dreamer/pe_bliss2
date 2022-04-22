#pragma once

#include <cstdint>
#include <list>
#include <span>
#include <system_error>
#include <type_traits>
#include <utility>

#include "buffers/input_buffer_interface.h"
#include "buffers/ref_buffer.h"
#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/data_directories.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/packed_byte_array.h"
#include "pe_bliss2/packed_byte_vector.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/dos_header.h"
#include "pe_bliss2/dos_stub.h"
#include "pe_bliss2/image_signature.h"
#include "pe_bliss2/file_header.h"
#include "pe_bliss2/optional_header.h"
#include "pe_bliss2/overlay.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/rich_header.h"
#include "pe_bliss2/section_table.h"
#include "pe_bliss2/section_data.h"
#include "utilities/safe_uint.h"

namespace pe_bliss
{

class packed_c_string;
class packed_utf16_string;
namespace detail
{
template<typename T>
concept packed_string_type = std::is_same_v<T, packed_utf16_string>
	|| std::is_same_v<T, packed_c_string>;
} //namespace detail

enum class image_errc
{
	too_many_sections = 1,
	too_many_rva_and_sizes,
	section_data_does_not_exist
};

std::error_code make_error_code(image_errc) noexcept;

class image
{
public:
	using section_data_list = std::list<section_data>;

public:
	[[nodiscard]] dos_header& get_dos_header() noexcept
	{
		return dos_header_;
	}

	[[nodiscard]] const dos_header& get_dos_header() const noexcept
	{
		return dos_header_;
	}

	[[nodiscard]] dos_stub& get_dos_stub() noexcept
	{
		return dos_stub_;
	}

	[[nodiscard]] const dos_stub& get_dos_stub() const noexcept
	{
		return dos_stub_;
	}

	[[nodiscard]] rich_header& get_rich_header() noexcept
	{
		return rich_header_;
	}

	[[nodiscard]] const rich_header& get_rich_header() const noexcept
	{
		return rich_header_;
	}

	[[nodiscard]] image_signature& get_image_signature() noexcept
	{
		return image_signature_;
	}

	[[nodiscard]] const image_signature& get_image_signature() const noexcept
	{
		return image_signature_;
	}

	[[nodiscard]] file_header& get_file_header() noexcept
	{
		return file_header_;
	}

	[[nodiscard]] const file_header& get_file_header() const noexcept
	{
		return file_header_;
	}

	[[nodiscard]] optional_header& get_optional_header() noexcept
	{
		return optional_header_;
	}

	[[nodiscard]] const optional_header& get_optional_header() const noexcept
	{
		return optional_header_;
	}

	[[nodiscard]] data_directories& get_data_directories() noexcept
	{
		return data_directories_;
	}

	[[nodiscard]] const data_directories& get_data_directories() const noexcept
	{
		return data_directories_;
	}

	[[nodiscard]] section_table& get_section_table() noexcept
	{
		return section_table_;
	}

	[[nodiscard]] const section_table& get_section_table() const noexcept
	{
		return section_table_;
	}
	
	[[nodiscard]] section_data_list& get_section_data_list() noexcept
	{
		return section_list_;
	}

	[[nodiscard]] const section_data_list& get_section_data_list() const noexcept
	{
		return section_list_;
	}

	[[nodiscard]] const overlay& get_overlay() const noexcept
	{
		return overlay_;
	}

	[[nodiscard]] overlay& get_overlay() noexcept
	{
		return overlay_;
	}

	[[nodiscard]] const buffers::ref_buffer& get_full_headers_buffer() const noexcept
	{
		return full_headers_buffer_;
	}

	[[nodiscard]] buffers::ref_buffer& get_full_headers_buffer() noexcept
	{
		return full_headers_buffer_;
	}

public:
	[[nodiscard]] bool is_64bit() const noexcept;
	[[nodiscard]] bool has_relocation() const noexcept;
	[[nodiscard]] bool is_loaded_to_memory() const noexcept
	{
		return loaded_to_memory_;
	}

public:
	void set_loaded_to_memory(bool loaded_to_memory) noexcept
	{
		loaded_to_memory_ = loaded_to_memory;
	}

public:
	using section_ref = std::pair<
		section_table::header_list::iterator, section_data_list::iterator>;
	using section_const_ref = std::pair<
		section_table::header_list::const_iterator, section_data_list::const_iterator>;

public:
	[[nodiscard]]
	section_ref section_from_reference(section_header& section_hdr) noexcept;
	[[nodiscard]]
	section_const_ref section_from_reference(const section_header& section_hdr) const noexcept;

public:
	[[nodiscard]]
	section_ref section_from_rva(rva_type rva,
		std::uint32_t data_size = 0);
	[[nodiscard]]
	section_const_ref section_from_rva(rva_type rva,
		std::uint32_t data_size = 0) const;

	[[nodiscard]]
	section_ref section_from_va(std::uint32_t va,
		std::uint32_t data_size = 0);
	[[nodiscard]]
	section_const_ref section_from_va(std::uint32_t va,
		std::uint32_t data_size = 0) const;
	[[nodiscard]]
	section_ref section_from_va(std::uint64_t va,
		std::uint32_t data_size = 0);
	[[nodiscard]]
	section_const_ref section_from_va(std::uint64_t va,
		std::uint32_t data_size = 0) const;

	[[nodiscard]]
	section_ref section_from_directory(data_directories::directory_type directory);
	[[nodiscard]]
	section_const_ref section_from_directory(data_directories::directory_type directory) const;

	[[nodiscard]]
	section_ref section_from_file_offset(std::uint32_t offset,
		std::uint32_t data_size = 0);
	[[nodiscard]]
	section_const_ref section_from_file_offset(std::uint32_t offset,
		std::uint32_t data_size = 0) const;

public:
	[[nodiscard]]
	std::uint32_t file_offset_to_rva(std::uint32_t offset) const;
	[[nodiscard]]
	std::uint32_t rva_to_file_offset(rva_type rva) const;

public:
	[[nodiscard]]
	buffers::input_buffer_ptr section_data_from_rva(rva_type rva,
		std::uint32_t data_size, bool include_headers = false,
		bool allow_virtual_data = false) const;
	[[nodiscard]]
	std::span<std::byte> section_data_from_rva(rva_type rva,
		std::uint32_t data_size, bool include_headers = false,
		bool allow_virtual_data = false);
	[[nodiscard]]
	buffers::input_buffer_ptr section_data_from_va(std::uint32_t va,
		std::uint32_t data_size, bool include_headers = false,
		bool allow_virtual_data = false) const;
	[[nodiscard]]
	std::span<std::byte> section_data_from_va(std::uint32_t va,
		std::uint32_t data_size, bool include_headers = false,
		bool allow_virtual_data = false);
	[[nodiscard]]
	buffers::input_buffer_ptr section_data_from_va(std::uint64_t va,
		std::uint32_t data_size, bool include_headers = false,
		bool allow_virtual_data = false) const;
	[[nodiscard]]
	std::span<std::byte> section_data_from_va(std::uint64_t va,
		std::uint32_t data_size, bool include_headers = false,
		bool allow_virtual_data = false);

public:
	[[nodiscard]]
	buffers::input_buffer_ptr section_data_from_rva(rva_type rva,
		bool include_headers = false) const;
	[[nodiscard]]
	std::span<std::byte> section_data_from_rva(rva_type rva,
		bool include_headers = false);
	[[nodiscard]]
	buffers::input_buffer_ptr section_data_from_va(std::uint32_t va,
		bool include_headers = false) const;
	[[nodiscard]]
	std::span<std::byte> section_data_from_va(std::uint32_t va,
		bool include_headers = false);
	[[nodiscard]]
	buffers::input_buffer_ptr section_data_from_va(std::uint64_t va,
		bool include_headers = false) const;
	[[nodiscard]]
	std::span<std::byte> section_data_from_va(std::uint64_t va,
		bool include_headers = false);

public:
	template<detail::packed_string_type PackedString = packed_c_string>
	[[nodiscard]]
	PackedString string_from_rva(rva_type rva,
		bool include_headers = false, bool allow_virtual_data = false) const;
	template<detail::packed_string_type PackedString = packed_c_string>
	void string_from_rva(rva_type rva, PackedString& str,
		bool include_headers = false, bool allow_virtual_data = false) const;

	template<detail::packed_string_type PackedString = packed_c_string>
	[[nodiscard]]
	PackedString string_from_va(std::uint32_t va,
		bool include_headers = false, bool allow_virtual_data = false) const;
	template<detail::packed_string_type PackedString = packed_c_string>
	void string_from_va(std::uint32_t va, PackedString& str,
		bool include_headers = false, bool allow_virtual_data = false) const;

	template<detail::packed_string_type PackedString = packed_c_string>
	[[nodiscard]]
	PackedString string_from_va(std::uint64_t va,
		bool include_headers = false, bool allow_virtual_data = false) const;
	template<detail::packed_string_type PackedString = packed_c_string>
	void string_from_va(std::uint64_t va, PackedString& str,
		bool include_headers = false, bool allow_virtual_data = false) const;

public:
	template<std::size_t MaxSize>
	packed_byte_array<MaxSize> byte_array_from_rva(rva_type rva,
		std::uint32_t size, bool include_headers, bool allow_virtual_data) const
	{
		packed_byte_array<MaxSize> result;
		byte_array_from_rva(rva, result, size, include_headers, allow_virtual_data);
		return result;
	}

	template<std::size_t MaxSize>
	void byte_array_from_rva(rva_type rva, packed_byte_array<MaxSize>& arr,
		std::uint32_t size, bool include_headers, bool allow_virtual_data) const
	{
		auto buf = section_data_from_rva(rva, size, include_headers, allow_virtual_data);
		arr.deserialize(*buf, size, allow_virtual_data);
	}

	template<detail::executable_pointer Va, std::size_t MaxSize>
	packed_byte_array<MaxSize> byte_array_from_va(Va va,
		std::uint32_t size, bool include_headers, bool allow_virtual_data) const
	{
		return byte_array_from_rva(address_converter(*this).va_to_rva(va),
			size, include_headers, allow_virtual_data);
	}

	template<detail::executable_pointer Va, std::size_t MaxSize>
	void byte_array_from_va(Va va, packed_byte_array<MaxSize>& arr,
		std::uint32_t size, bool include_headers, bool allow_virtual_data) const
	{
		byte_array_from_rva(address_converter(*this).va_to_rva(va), arr, size,
			include_headers, allow_virtual_data);
	}

public:
	packed_byte_vector byte_vector_from_rva(rva_type rva,
		std::uint32_t size, bool include_headers, bool allow_virtual_data) const;
	void byte_vector_from_rva(rva_type rva, packed_byte_vector& arr,
		std::uint32_t size, bool include_headers, bool allow_virtual_data) const;

	template<detail::executable_pointer Va>
	packed_byte_vector byte_vector_from_va(Va va,
		std::uint32_t size, bool include_headers, bool allow_virtual_data) const
	{
		return byte_vector_from_rva(address_converter(*this).va_to_rva(va),
			size, include_headers, allow_virtual_data);
	}

	template<detail::executable_pointer Va>
	void byte_vector_from_va(Va va, packed_byte_vector& arr,
		std::uint32_t size, bool include_headers, bool allow_virtual_data) const
	{
		byte_vector_from_rva(address_converter(*this).va_to_rva(va), arr, size,
			include_headers, allow_virtual_data);
	}

public:
	template<detail::standard_layout T>
	[[nodiscard]]
	packed_struct<T> struct_from_rva(uint32_t rva, bool include_headers = false,
		bool allow_virtual_data = false) const
	{
		packed_struct<T> value{};
		auto buf = section_data_from_rva(rva,
			value.packed_size, include_headers, allow_virtual_data);
		value.deserialize(*buf, allow_virtual_data);
		return value;
	}

	template<detail::standard_layout T, detail::executable_pointer Va>
	[[nodiscard]]
	packed_struct<T> struct_from_va(Va va, bool include_headers = false,
		bool allow_virtual_data = false) const
	{
		packed_struct<T> value{};
		auto buf = section_data_from_va(va,
			value.packed_size, include_headers, allow_virtual_data);
		value.deserialize(*buf, allow_virtual_data);
		return value;
	}

	template<detail::standard_layout T>
	packed_struct<T>& struct_from_rva(uint32_t rva, packed_struct<T>& value,
		bool include_headers = false, bool allow_virtual_data = false) const
	{
		auto buf = section_data_from_rva(rva,
			value.packed_size, include_headers, allow_virtual_data);
		value.deserialize(*buf, allow_virtual_data);
		return value;
	}

	template<detail::executable_pointer Va, detail::standard_layout T>
	packed_struct<T>& struct_from_va(Va va, packed_struct<T>& value,
		bool include_headers = false, bool allow_virtual_data = false) const
	{
		auto buf = section_data_from_va(va,
			value.packed_size, include_headers, allow_virtual_data);
		value.deserialize(*buf, allow_virtual_data);
		return value;
	}

public:
	template<detail::standard_layout T>
	rva_type struct_to_rva(uint32_t rva, const packed_struct<T>& value,
		bool include_headers = false, bool write_virtual_part = false,
		bool cut_if_does_not_fit = false)
	{
		if (!value.physical_size() && !write_virtual_part)
			return rva;

		auto buf = section_data_from_rva(rva,
			static_cast<std::uint32_t>(
				write_virtual_part ? value.packed_size : value.physical_size()),
			include_headers, cut_if_does_not_fit);

		auto size = cut_if_does_not_fit
			? value.serialize_until(buf.data(), buf.size_bytes(), write_virtual_part)
			: value.serialize(buf.data(), buf.size_bytes(), write_virtual_part);

		return (utilities::safe_uint(rva) + size).value();
	}

	template<detail::executable_pointer Va, detail::standard_layout T>
	Va struct_to_va(Va va, const packed_struct<T>& value,
		bool include_headers = false, bool write_virtual_part = false,
		bool cut_if_does_not_fit = false)
	{
		if (!value.physical_size() && !write_virtual_part)
			return va;

		auto buf = section_data_from_va(va,
			static_cast<std::uint32_t>(
				write_virtual_part ? value.packed_size : value.physical_size()),
			include_headers, cut_if_does_not_fit);

		auto size = cut_if_does_not_fit
			? value.serialize_until(buf.data(), buf.size_bytes(), write_virtual_part)
			: value.serialize(buf.data(), buf.size_bytes(), write_virtual_part);

		return (utilities::safe_uint(va) + size).value();
	}

	template<detail::standard_layout T>
	rva_type struct_to_rva(uint32_t rva, const T& value, bool include_headers = false,
		bool cut_if_does_not_fit = false)
	{
		packed_struct wrapper(value);
		return struct_to_rva(rva, wrapper, include_headers, true, cut_if_does_not_fit);
	}

	template<detail::executable_pointer Va, detail::standard_layout T>
	Va struct_to_va(Va va, const T& value, bool include_headers = false,
		bool cut_if_does_not_fit = false)
	{
		packed_struct wrapper(value);
		return struct_to_va(va, wrapper, include_headers, true, cut_if_does_not_fit);
	}

public:
	template<detail::standard_layout T>
	rva_type struct_to_file_offset(const packed_struct<T>& value,
		bool include_headers = false, bool write_virtual_part = false)
	{
		return struct_to_rva(absolute_offset_to_rva(value),
			value, include_headers, write_virtual_part);
	}

public:
	template<detail::packed_string_type PackedString = packed_c_string>
	rva_type string_to_rva(rva_type rva, const PackedString& str,
		bool include_headers = false, bool write_virtual_part = false);
	template<detail::packed_string_type PackedString = packed_c_string>
	std::uint32_t string_to_va(std::uint32_t va, const PackedString& str,
		bool include_headers = false, bool write_virtual_part = false);
	template<detail::packed_string_type PackedString = packed_c_string>
	std::uint64_t string_to_va(std::uint64_t va, const PackedString& str,
		bool include_headers = false, bool write_virtual_part = false);

public:
	template<detail::packed_string_type PackedString = packed_c_string>
	rva_type string_to_file_offset(const PackedString& str,
		bool include_headers = false, bool write_virtual_part = false);

public:
	template<typename ArrayOrVector>
	rva_type byte_array_to_rva(rva_type rva, const ArrayOrVector& arr,
		bool include_headers = false, bool write_virtual_part = false)
	{
		if (!arr.data_size() || (!arr.physical_size() && !write_virtual_part))
			return rva;

		auto buf = section_data_from_rva(rva, include_headers);
		return rva + static_cast<rva_type>(
			arr.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
	}

	template<detail::executable_pointer Va, typename ArrayOrVector>
	Va byte_array_to_va(Va va, const ArrayOrVector& arr,
		bool include_headers = false, bool write_virtual_part = false)
	{
		if (!arr.data_size() || (!arr.physical_size() && !write_virtual_part))
			return va;

		auto buf = section_data_from_va(va, include_headers);
		return va + static_cast<Va>(
			arr.serialize(buf.data(), buf.size_bytes(), write_virtual_part));
	}

public:
	template<typename ArrayOrVector>
	rva_type byte_array_to_file_offset(const ArrayOrVector& arr,
		bool include_headers = false, bool write_virtual_part = false)
	{
		return byte_array_to_rva(absolute_offset_to_rva(arr), arr,
			include_headers, write_virtual_part);
	}

public:
	rva_type buffer_to_rva(rva_type rva, const buffers::ref_buffer& buf,
		bool include_headers = false);
	std::uint32_t buffer_to_va(std::uint32_t va, const buffers::ref_buffer& buf,
		bool include_headers = false);
	std::uint64_t buffer_to_va(std::uint64_t va, const buffers::ref_buffer& buf,
		bool include_headers = false);

public:
	rva_type buffer_to_file_offset(const buffers::ref_buffer& buf,
		bool include_headers = false);

public:
	[[nodiscard]]
	std::uint32_t section_data_length_from_rva(rva_type rva,
		bool include_headers = false, bool allow_virtual_data = false) const;
	[[nodiscard]]
	std::uint32_t section_data_length_from_va(std::uint32_t va,
		bool include_headers = false, bool allow_virtual_data = false) const;
	[[nodiscard]]
	std::uint32_t section_data_length_from_va(std::uint64_t va,
		bool include_headers = false, bool allow_virtual_data = false) const;

public:
	template<typename PackedStruct>
	rva_type absolute_offset_to_rva(const PackedStruct& obj) const
	{
		utilities::safe_uint<std::uint32_t> offset;
		offset += obj.get_state().absolute_offset();
		return file_offset_to_rva(offset.value());
	}

	rva_type absolute_offset_to_rva(const buffers::input_buffer_interface& buf) const;

public:
	void update_number_of_sections();
	void update_image_size();
	void set_number_of_data_directories(std::uint32_t number);
	std::uint32_t strip_data_directories(std::uint32_t min_count);
	void copy_referenced_section_memory();

private:
	bool loaded_to_memory_ = false;
	dos_header dos_header_;
	dos_stub dos_stub_;
	rich_header rich_header_;
	image_signature image_signature_;
	file_header file_header_;
	optional_header optional_header_;
	data_directories data_directories_;
	section_table section_table_;
	section_data_list section_list_;
	overlay overlay_;
	buffers::ref_buffer full_headers_buffer_;
};

} //namespace pe_bliss

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::image_errc> : true_type {};
} //namespace std
