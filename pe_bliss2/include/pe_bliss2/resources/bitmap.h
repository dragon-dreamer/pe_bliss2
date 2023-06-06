#pragma once

#include "buffers/ref_buffer.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/detail/resources/bitmap.h"

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
	buffers::ref_buffer& get_buffer() noexcept
	{
		return buf_;
	}

	[[nodiscard]]
	const buffers::ref_buffer& get_buffer() const noexcept
	{
		return buf_;
	}

private:
	file_header_type fh_;
	info_header_type ih_;
	buffers::ref_buffer buf_;
};

} //namespace pe_bliss::resources
