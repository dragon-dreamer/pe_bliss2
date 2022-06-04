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
