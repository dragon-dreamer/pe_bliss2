#pragma once

#include <cstdint>
#include <span>
#include <type_traits>
#include <utility>

#include "buffers/input_buffer_interface.h"
#include "buffers/ref_buffer.h"
#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/core/image_signature.h"
#include "pe_bliss2/core/optional_header.h"
#include "pe_bliss2/core/overlay.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/image/rva_file_offset_converter.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/packed_byte_array.h"
#include "pe_bliss2/packed_byte_vector.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_stub.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/section/section_table.h"
#include "pe_bliss2/section/section_data.h"
#include "utilities/safe_uint.h"

namespace pe_bliss::image
{

class [[nodiscard]] image
{
public:
	[[nodiscard]] inline dos::dos_header& get_dos_header() & noexcept;
	[[nodiscard]] inline const dos::dos_header& get_dos_header() const& noexcept;
	[[nodiscard]] inline dos::dos_header get_dos_header() && noexcept;

	[[nodiscard]] inline dos::dos_stub& get_dos_stub() & noexcept;
	[[nodiscard]] inline const dos::dos_stub& get_dos_stub() const& noexcept;
	[[nodiscard]] inline dos::dos_stub get_dos_stub() && noexcept;

	[[nodiscard]] inline core::image_signature& get_image_signature() & noexcept;
	[[nodiscard]] inline const core::image_signature& get_image_signature() const& noexcept;
	[[nodiscard]] inline core::image_signature get_image_signature() && noexcept;

	[[nodiscard]] inline core::file_header& get_file_header() & noexcept;
	[[nodiscard]] inline const core::file_header& get_file_header() const& noexcept;
	[[nodiscard]] inline core::file_header get_file_header() && noexcept;

	[[nodiscard]] inline core::optional_header& get_optional_header() & noexcept;
	[[nodiscard]] inline const core::optional_header& get_optional_header() const& noexcept;
	[[nodiscard]] inline core::optional_header get_optional_header() && noexcept;

	[[nodiscard]] inline core::data_directories& get_data_directories() & noexcept;
	[[nodiscard]] inline const core::data_directories& get_data_directories() const& noexcept;
	[[nodiscard]] inline core::data_directories get_data_directories() && noexcept;

	[[nodiscard]] inline section::section_table& get_section_table() & noexcept;
	[[nodiscard]] inline const section::section_table& get_section_table() const& noexcept;
	[[nodiscard]] inline section::section_table get_section_table() && noexcept;

	[[nodiscard]] inline section::section_data_list& get_section_data_list() & noexcept;
	[[nodiscard]] inline const section::section_data_list& get_section_data_list() const& noexcept;
	[[nodiscard]] inline section::section_data_list get_section_data_list() && noexcept;

	[[nodiscard]] inline core::overlay& get_overlay() & noexcept;
	[[nodiscard]] inline const core::overlay& get_overlay() const& noexcept;
	[[nodiscard]] inline core::overlay get_overlay() && noexcept;

	[[nodiscard]] inline buffers::ref_buffer& get_full_headers_buffer() & noexcept;
	[[nodiscard]] inline const buffers::ref_buffer& get_full_headers_buffer() const& noexcept;
	[[nodiscard]] inline buffers::ref_buffer get_full_headers_buffer() && noexcept;

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
		auto buf = section_data_from_rva(*this, rva,
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
		auto buf = section_data_from_va(*this, va,
			value.packed_size, include_headers, allow_virtual_data);
		value.deserialize(*buf, allow_virtual_data);
		return value;
	}

	template<detail::standard_layout T>
	packed_struct<T>& struct_from_rva(uint32_t rva, packed_struct<T>& value,
		bool include_headers = false, bool allow_virtual_data = false) const
	{
		auto buf = section_data_from_rva(*this, rva,
			value.packed_size, include_headers, allow_virtual_data);
		value.deserialize(*buf, allow_virtual_data);
		return value;
	}

	template<detail::executable_pointer Va, detail::standard_layout T>
	packed_struct<T>& struct_from_va(Va va, packed_struct<T>& value,
		bool include_headers = false, bool allow_virtual_data = false) const
	{
		auto buf = section_data_from_va(*this, va,
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

		auto buf = section_data_from_rva(*this, rva,
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

		auto buf = section_data_from_va(*this, va,
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
		return struct_to_rva(absolute_offset_to_rva(*this, value),
			value, include_headers, write_virtual_part);
	}

public:
	template<typename ArrayOrVector>
	rva_type byte_array_to_rva(rva_type rva, const ArrayOrVector& arr,
		bool include_headers = false, bool write_virtual_part = false)
	{
		if (!arr.data_size() || (!arr.physical_size() && !write_virtual_part))
			return rva;

		auto buf = section_data_from_rva(*this, rva, include_headers);
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
		return byte_array_to_rva(absolute_offset_to_rva(*this, arr), arr,
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
	void update_number_of_sections();
	void update_image_size();
	void set_number_of_data_directories(std::uint32_t number);
	std::uint32_t strip_data_directories(std::uint32_t min_count);
	void copy_referenced_section_memory();

private:
	bool loaded_to_memory_ = false;
	dos::dos_header dos_header_;
	dos::dos_stub dos_stub_;
	core::image_signature image_signature_;
	core::file_header file_header_;
	core::optional_header optional_header_;
	core::data_directories data_directories_;
	section::section_table section_table_;
	section::section_data_list section_list_;
	core::overlay overlay_;
	buffers::ref_buffer full_headers_buffer_;
};

} //namespace pe_bliss::image

#include "pe_bliss2/detail/image/image-inl.h"
