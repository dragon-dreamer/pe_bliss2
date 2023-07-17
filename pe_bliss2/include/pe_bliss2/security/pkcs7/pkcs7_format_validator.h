#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <system_error>
#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/crypto_algorithms.h"
#include "pe_bliss2/security/pkcs7/pkcs7.h"

namespace pe_bliss::security::pkcs7
{

enum class pkcs7_format_validator_errc
{
	invalid_signed_data_oid,
	invalid_signed_data_version,
	invalid_signer_count,
	non_matching_digest_algorithm,
	invalid_signer_info_version,
	absent_message_digest,
	invalid_message_digest,
	absent_content_type,
	invalid_content_type,
	invalid_signing_time
};

std::error_code make_error_code(pkcs7_format_validator_errc) noexcept;
} //namespace pe_bliss::security::pkcs7

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::pkcs7::pkcs7_format_validator_errc> : true_type {};
} //namespace std

namespace pe_bliss::security::pkcs7
{
namespace impl
{
constexpr std::array signed_data_oid {
	1u, 2u, 840u, 113549u, 1u, 7u, 2u
};

constexpr std::uint32_t signed_data_version = 1u;
constexpr std::uint32_t signer_info_version = 1u;
} //namespace impl

template<typename RangeType, typename ContextType>
void validate(const pe_bliss::security::pkcs7::pkcs7<RangeType, ContextType>& signature,
	error_list& errors)
{
	const auto& content_info = signature.get_content_info();
	if (!std::ranges::equal(content_info.content_type.container, impl::signed_data_oid))
		errors.add_error(pkcs7_format_validator_errc::invalid_signed_data_oid);

	if (content_info.data.version != impl::signed_data_version)
		errors.add_error(pkcs7_format_validator_errc::invalid_signed_data_version);

	if (content_info.data.digest_algorithms.size()
		!= content_info.data.signer_infos.size())
	{
		errors.add_error(pkcs7_format_validator_errc::invalid_signer_count);
	}
	else
	{
		for (std::size_t i = 0; i != content_info.data.signer_infos.size(); ++i)
		{
			const auto& signer_info = content_info.data.signer_infos[i];
			if (!algorithm_id_equals(content_info.data.digest_algorithms[i],
				signer_info.digest_algorithm))
			{
				errors.add_error(pkcs7_format_validator_errc::non_matching_digest_algorithm);
			}

			if (signer_info.version != impl::signer_info_version)
				errors.add_error(pkcs7_format_validator_errc::invalid_signer_info_version);
		}
	}
}

template<typename RangeType>
void validate_authenticated_attributes(
	const pe_bliss::security::pkcs7::attribute_map<RangeType>& authenticated_attributes,
	error_list& errors)
{
	try
	{
		if (!authenticated_attributes.get_message_digest())
			errors.add_error(pkcs7_format_validator_errc::absent_message_digest);
	}
	catch (const pe_error&)
	{
		errors.add_error(pkcs7_format_validator_errc::invalid_message_digest);
	}

	try
	{
		auto content_type = authenticated_attributes.get_content_type();
		if (!content_type)
			errors.add_error(pkcs7_format_validator_errc::absent_content_type);
	}
	catch (const pe_error&)
	{
		errors.add_error(pkcs7_format_validator_errc::invalid_content_type);
	}

	try
	{
		(void)authenticated_attributes.get_signing_time();
	}
	catch (const pe_error&)
	{
		errors.add_error(pkcs7_format_validator_errc::invalid_signing_time);
	}
}

} //namespace pe_bliss::security
