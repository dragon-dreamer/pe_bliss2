#pragma once

#include <optional>
#include <system_error>
#include <type_traits>
#include <vector>

#include "buffers/input_buffer_interface.h"

#include "pe_bliss2/detail/security/image_security_directory.h"
#include "pe_bliss2/security/authenticode_pkcs7.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"

namespace pe_bliss::security
{

enum class authenticode_loader_errc
{
	unsupported_certificate_type = 1,
	invalid_authenticode_asn1_der,
	buffer_is_not_contiguous,
	virtual_authenticode_asn1_der_data
};

std::error_code make_error_code(authenticode_loader_errc) noexcept;

// authenticode_span_range_type is safe to use as RangeType only
// if the buffer is fully copied to memory (otherwise the function will throw).
template<typename RangeType>
[[nodiscard]]
authenticode_pkcs7<RangeType> load_authenticode_signature(
	buffers::input_buffer_interface& buffer);

template<typename RangeType>
[[nodiscard]]
authenticode_pkcs7<RangeType> load_authenticode_signature(
	buffers::input_buffer_interface& buffer,
	const detail::security::win_certificate& certificate_info);

// Double-signing support
template<typename TargetRangeType, typename RangeType>
[[nodiscard]]
std::vector<authenticode_pkcs7<TargetRangeType>> load_nested_signatures(
	const pkcs7::attribute_map<RangeType>& unauthenticated_attributes);

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_loader_errc> : true_type {};
} //namespace std
