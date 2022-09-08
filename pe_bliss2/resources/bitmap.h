#pragma once

#include "buffers/input_buffer_interface.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/detail/resources/bitmap.h"

namespace buffers
{
class output_buffer_interface;
} //namespace buffers

namespace pe_bliss::resources
{

class [[nodiscard]] bitmap
{
public:
	using file_header_type = packed_struct<detail::resources::bitmap_file_header>;
	using info_header_type = packed_struct<detail::resources::bitmap_info_header>;

public:
	[[nodiscard]]
	file_header_type& get_file_header() noexcept
	{
		return fh_;
	}

	[[nodiscard]]
	const file_header_type& get_file_header() const noexcept
	{
		return fh_;
	}

	[[nodiscard]]
	info_header_type& get_info_header() noexcept
	{
		return ih_;
	}

	[[nodiscard]]
	const info_header_type& get_info_header() const noexcept
	{
		return ih_;
	}

	[[nodiscard]]
	buffers::input_buffer_ptr& get_buffer() noexcept
	{
		return buf_;
	}

	[[nodiscard]]
	const buffers::input_buffer_ptr& get_buffer() const noexcept
	{
		return buf_;
	}

	void serialize(buffers::output_buffer_interface& output,
		bool write_virtual_part);

private:
	file_header_type fh_;
	info_header_type ih_;
	buffers::input_buffer_ptr buf_;
};

} //namespace pe_bliss::resources
