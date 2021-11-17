#pragma once

#include <cstdint>
#include <list>
#include <utility>
#include <variant>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/detail/tls/image_tls_directory.h"
#include "pe_bliss2/detail/error_list.h"
#include "pe_bliss2/detail/packed_struct.h"

namespace pe_bliss::tls
{

template<typename Directory>
class tls_directory_base
{
public:
	using va_type = decltype(Directory::address_of_callbacks);
	using packed_descriptor_type = detail::packed_struct<Directory>;

	using index_type = std::uint32_t;
	using callback_type = detail::packed_struct<va_type>;
	using callback_list_type = std::list<callback_type>;

public:
	[[nodiscard]]
	const packed_descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	packed_descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const buffers::ref_buffer& get_raw_data() const noexcept
	{
		return raw_data_;
	}

	[[nodiscard]]
	buffers::ref_buffer& get_raw_data() noexcept
	{
		return raw_data_;
	}

	[[nodiscard]]
	const callback_list_type& get_callbacks() const noexcept
	{
		return callbacks_;
	}

	[[nodiscard]]
	callback_list_type& get_callbacks() noexcept
	{
		return callbacks_;
	}

	void set_raw_data_size(std::uint32_t size) noexcept
	{
		descriptor_.start_address_of_raw_data = 0;
		descriptor_.end_address_of_raw_data = size;
	}

private:
	packed_descriptor_type descriptor_;
	buffers::ref_buffer raw_data_;
	callback_list_type callbacks_;
};

template<typename Directory>
class tls_directory_details_base
	: public tls_directory_base<Directory>
	, public detail::error_list
{
public:
	using tls_directory_base<Directory>::tls_directory_base;
};

using tls_directory_details32 = tls_directory_details_base<detail::tls::image_tls_directory32>;
using tls_directory_details64 = tls_directory_details_base<detail::tls::image_tls_directory64>;
using tls_directory32 = tls_directory_base<detail::tls::image_tls_directory32>;
using tls_directory64 = tls_directory_base<detail::tls::image_tls_directory64>;

using tls_directory = std::variant<tls_directory32, tls_directory64>;
using tls_directory_details = std::variant<tls_directory_details32, tls_directory_details64>;

} //namespace pe_bliss::tls
