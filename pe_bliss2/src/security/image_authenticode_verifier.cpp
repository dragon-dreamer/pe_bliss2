#include "pe_bliss2/security/image_authenticode_verifier.h"

#include <type_traits>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/security/authenticode_loader.h"
#include "pe_bliss2/security/authenticode_verifier.h"
#include "pe_bliss2/security/security_directory_loader.h"

#include "simple_asn1/decode.h"

namespace pe_bliss::security
{

std::optional<image_authenticode_check_status> verify_authenticode(
	const image::image& instance,
	const authenticode_verification_options& opts)
{
	const auto sec_dir = load(instance);
	if (!sec_dir)
		return {};

	return verify_authenticode(instance, *sec_dir, opts);
}

template<typename... Bases>
std::optional<image_authenticode_check_status> verify_authenticode(
	const image::image& instance,
	const security_directory_base<Bases...>& sec_dir,
	const authenticode_verification_options& opts)
{
	std::optional<image_authenticode_check_status> optional_result;
	if constexpr (std::is_same_v<security_directory_base<Bases...>,
		security_directory_details>)
	{
		if (sec_dir.has_errors())
		{
			optional_result.emplace().
				authenticode_status.root.authenticode_format_errors.add_error(
					authenticode_verifier_errc::image_security_directory_has_errors);
			return optional_result;
		}
	}

	if (sec_dir.get_entries().empty())
		return optional_result;

	auto& result = optional_result.emplace();

	authenticode_pkcs7<span_range_type> authenticode;

	try
	{
		authenticode = load_authenticode_signature<span_range_type>(
			*sec_dir.get_entries()[0].get_certificate().data());
	}
	catch (const asn1::parse_error&)
	{
		result.error = std::current_exception();
		result.authenticode_status.root.authenticode_format_errors.add_error(
			authenticode_verifier_errc::invalid_authenticode_signature_format);
		return optional_result;
	}
	catch (const pe_error&)
	{
		result.error = std::current_exception();
		result.authenticode_status.root.authenticode_format_errors.add_error(
			authenticode_verifier_errc::invalid_authenticode_signature_format);
		return optional_result;
	}

	result.authenticode_status = verify_authenticode_full<span_range_type>(
		authenticode, instance, opts);
	return optional_result;
}

template std::optional<image_authenticode_check_status>
verify_authenticode<>(
	const image::image& instance,
	const security_directory_base<>& sec_dir,
	const authenticode_verification_options& opts);
template std::optional<image_authenticode_check_status>
verify_authenticode<error_list>(
	const image::image& instance,
	const security_directory_base<error_list>& sec_dir,
	const authenticode_verification_options& opts);

} //namespace pe_bliss::security
