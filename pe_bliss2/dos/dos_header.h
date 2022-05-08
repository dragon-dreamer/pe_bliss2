#pragma once

#include <cstdint>

#include "pe_bliss2/detail/image_dos_header.h"
#include "pe_bliss2/detail/packed_struct_base.h"

namespace buffers
{
class input_buffer_interface;
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::dos
{

class dos_header : public detail::packed_struct_base<detail::image_dos_header>
{
public:
	///DOS header magic value ('MZ')
	static constexpr std::uint16_t mz_magic_value = 0x5a4d;
	///Minimum valid e_lfanew value
	static constexpr std::uint32_t min_e_lfanew = 4;
	///Maximum valid e_lfanew value (safety limit)
	static constexpr std::uint32_t max_e_lfanew = 10485760; //10 Mb

public:
	//When deserializing, buffer should point to the start of the image
	void deserialize(buffers::input_buffer_interface& buf,
		bool allow_virtual_memory = false);
	void serialize(buffers::output_buffer_interface& buf,
		bool write_virtual_part = true) const;
};

} //namespace pe_bliss::dos
