#include "pe_bliss2/security/x500/flat_distinguished_name.h"

#include <string>
#include <system_error>
#include <utility>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/byte_range_types.h"

#include "simple_asn1/crypto/crypto_common_spec.h"
#include "simple_asn1/crypto/x520/types.h"
#include "simple_asn1/der_decode.h"
#include "simple_asn1/spec.h"

namespace
{
struct distinguished_name_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "distinguished_name";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::security::x500::distinguished_name_errc;
		switch (static_cast<pe_bliss::security::x500::distinguished_name_errc>(ev))
		{
		case duplicate_dn_attributes:
			return "Distinguished name has duplicate attributes";
		case invalid_rdn_attribute_value:
			return "Relative distinguished name attribute value ASN.1 DER is not valid";
		default:
			return {};
		}
	}
};

const distinguished_name_error_category distinguished_name_error_category_instance;

} //namespace

namespace pe_bliss::security::x500
{

std::error_code make_error_code(distinguished_name_errc e) noexcept
{
	return { static_cast<int>(e), distinguished_name_error_category_instance };
}

template<typename RangeType>
flat_distinguished_name<RangeType>::flat_distinguished_name(
	const security::pkcs7::signer_info_ref_pkcs7<range_type>& ref)
{
	build(ref.get_underlying().issuer_and_sn.issuer.value);
}

template<typename RangeType>
flat_distinguished_name<RangeType>::flat_distinguished_name(
	const security::pkcs7::signer_info_ref_cms<range_type>& ref)
{
	const auto& sid = ref.get_underlying().sid;
	if (const auto* issuer = std::get_if<
		asn1::crypto::pkcs7::issuer_and_serial_number<range_type>>(&sid); issuer)
	{
		build(issuer->issuer.value);
	}
}

template<typename RangeType>
template<std::size_t N>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_directory_string(
		const std::array<std::uint32_t, N>& oid) const
{
	std::optional<directory_string_type> result;
	if (auto it = parts_.find(oid); it != parts_.end())
	{
		try
		{
			const auto str = asn1::der::decode<asn1::crypto::directory_string,
				asn1::spec::crypto::directory_string<"RDN">>(it->second.begin(), it->second.end());
			std::visit([&result](auto& moved) { result = std::move(moved); }, std::move(str));
		}
		catch (const asn1::parse_error&)
		{
			std::throw_with_nested(pe_error(distinguished_name_errc::invalid_rdn_attribute_value));
		}
	}

	return result;
}

template<typename RangeType>
template<std::size_t N>
std::optional<std::string> flat_distinguished_name<RangeType>::get_printable_string(
	const std::array<std::uint32_t, N>& oid) const
{
	std::optional<std::string> result;
	if (auto it = parts_.find(oid); it != parts_.end())
	{
		try
		{
			asn1::der::decode<asn1::spec::printable_string<>>(
				it->second.begin(), it->second.end(), result.emplace());
		}
		catch (const asn1::parse_error&)
		{
			std::throw_with_nested(pe_error(distinguished_name_errc::invalid_rdn_attribute_value));
		}
	}

	return result;
}

template<typename RangeType>
void flat_distinguished_name<RangeType>::build(
	const std::vector<asn1::crypto::relative_distinguished_name_type<range_type>>& dn)
{
	for (const asn1::crypto::relative_distinguished_name_type<range_type>& rdn : dn)
	{
		for (const asn1::crypto::attribute_value_assertion<range_type>& attr : rdn)
		{
			if (!parts_.try_emplace(attr.attribute_type.container, attr.attribute_value).second)
				throw pe_error(distinguished_name_errc::duplicate_dn_attributes);
		}
	}
}

template<typename RangeType>
template<typename T1, typename T2>
constexpr bool flat_distinguished_name<RangeType>::comparer::operator()(
	const T1& l, const T2& r) const noexcept
{
	return std::lexicographical_compare(
		l.begin(), l.end(), r.begin(), r.end());
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_common_name() const
{
	return get_directory_string(asn1::crypto::x520::id_at_common_name);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_surname() const
{
	return get_directory_string(asn1::crypto::x520::id_at_surname);
}

template<typename RangeType>
std::optional<std::string> flat_distinguished_name<RangeType>::get_serial_number() const
{
	return get_printable_string(asn1::crypto::x520::id_at_serial_number);
}

template<typename RangeType>
std::optional<std::string> flat_distinguished_name<RangeType>::get_country_name() const
{
	return get_printable_string(asn1::crypto::x520::id_at_country_name);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_locality_name() const
{
	return get_directory_string(asn1::crypto::x520::id_at_locality_name);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_state_or_province_name() const
{
	return get_directory_string(asn1::crypto::x520::id_at_state_or_province_name);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_organization_name() const
{
	return get_directory_string(asn1::crypto::x520::id_at_organization_name);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_organizational_unit_name() const
{
	return get_directory_string(asn1::crypto::x520::id_at_organizational_unit_name);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_title() const
{
	return get_directory_string(asn1::crypto::x520::id_at_title);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_name() const
{
	return get_directory_string(asn1::crypto::x520::id_at_name);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_given_name() const
{
	return get_directory_string(asn1::crypto::x520::id_at_given_name);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_initials() const
{
	return get_directory_string(asn1::crypto::x520::id_at_initials);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_generation_qualifier() const
{
	return get_directory_string(asn1::crypto::x520::id_at_generation_qualifier);
}

template<typename RangeType>
std::optional<typename flat_distinguished_name<RangeType>::directory_string_type>
	flat_distinguished_name<RangeType>::get_pseudonim() const
{
	return get_directory_string(asn1::crypto::x520::id_at_pseudonim);
}

template class flat_distinguished_name<span_range_type>;
template class flat_distinguished_name<vector_range_type>;

} //namespace pe_bliss::security::x500
