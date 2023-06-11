#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/detail/tls/image_tls_directory.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_struct.h"

namespace pe_bliss::tls
{

enum class tls_directory_errc
{
	too_big_alignment_value = 1,
	invalid_alignment_value
};

std::error_code make_error_code(tls_directory_errc) noexcept;

template<typename Va>
using tls_callback = packed_struct<Va>;

template<typename Va>
class [[nodiscard]] tls_callback_details
	: public packed_struct<Va>
	, public error_list
{
public:
	tls_callback_details() = default;
	tls_callback_details(packed_struct<Va> other)
		: packed_struct<Va>(other)
	{
	}
};

template<typename Directory, typename TlsCallback>
class [[nodiscard]] tls_directory_base
	: public detail::packed_struct_base<Directory>
{
public:
	using va_type = decltype(Directory::address_of_callbacks);

	using index_type = std::uint32_t;
	using callback_type = TlsCallback;
	using callback_list_type = std::vector<callback_type>;

public:
	[[nodiscard]]
	const buffers::ref_buffer& get_raw_data() const noexcept;
	[[nodiscard]]
	buffers::ref_buffer& get_raw_data() noexcept;

	[[nodiscard]]
	const callback_list_type& get_callbacks() const& noexcept;
	[[nodiscard]]
	callback_list_type& get_callbacks() & noexcept;
	[[nodiscard]]
	callback_list_type get_callbacks() && noexcept;

	[[nodiscard]]
	std::uint32_t get_max_type_alignment() const noexcept;
	void set_max_type_alignment(std::uint32_t alignment);

private:
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

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::tls::tls_directory_errc> : true_type {};
} //namespace std

#include "pe_bliss2/tls/tls_directory-inl.h"
