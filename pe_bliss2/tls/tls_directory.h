#pragma once

#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/detail/tls/image_tls_directory.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_struct.h"

namespace pe_bliss::tls
{

template<typename Va>
using tls_callback = packed_struct<Va>;

template<typename Va>
class [[nodiscard]] tls_callback_details
	: public packed_struct<Va>
	, public error_list
{
};

template<typename Directory, typename TlsCallback>
class [[nodiscard]] tls_directory_base
{
public:
	using va_type = decltype(Directory::address_of_callbacks);
	using packed_descriptor_type = packed_struct<Directory>;

	using index_type = std::uint32_t;
	using callback_type = TlsCallback;
	using callback_list_type = std::vector<callback_type>;

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
	const callback_list_type& get_callbacks() const & noexcept
	{
		return callbacks_;
	}

	[[nodiscard]]
	callback_list_type& get_callbacks() & noexcept
	{
		return callbacks_;
	}

	[[nodiscard]]
	callback_list_type get_callbacks() && noexcept
	{
		return std::move(callbacks_);
	}

private:
	packed_descriptor_type descriptor_;
	buffers::ref_buffer raw_data_;
	callback_list_type callbacks_;
};

template<typename Directory, typename TlsCallback>
class [[nodiscard]] tls_directory_details_base
	: public tls_directory_base<Directory, TlsCallback>
	, public error_list
{
};

using tls_directory_details32 = tls_directory_details_base<detail::tls::image_tls_directory32,
	tls_callback_details<std::uint32_t>>;
using tls_directory_details64 = tls_directory_details_base<detail::tls::image_tls_directory64,
	tls_callback_details<std::uint64_t>>;
using tls_directory32 = tls_directory_base<detail::tls::image_tls_directory32,
	tls_callback<std::uint32_t>>;
using tls_directory64 = tls_directory_base<detail::tls::image_tls_directory64,
	tls_callback<std::uint64_t>>;

using tls_directory = std::variant<tls_directory32, tls_directory64>;
using tls_directory_details = std::variant<tls_directory_details32, tls_directory_details64>;

} //namespace pe_bliss::tls
