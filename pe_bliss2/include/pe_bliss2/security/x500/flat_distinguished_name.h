#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <system_error>
#include <variant>
#include <vector>

#include "pe_bliss2/security/pkcs7/signer_info.h"

#include "simple_asn1/crypto/crypto_common_types.h"
#include "simple_asn1/crypto/pkcs7/cms/types.h"

namespace pe_bliss::security::x500
{

enum class distinguished_name_errc
{
	invalid_rdn_attribute_value = 1
};

std::error_code make_error_code(distinguished_name_errc) noexcept;

template<typename RangeType>
class [[nodiscard]] flat_distinguished_name
{
public:
	using range_type = RangeType;
	using directory_string_type = std::variant<std::string, std::u16string, std::u32string>;

	struct comparer final
	{
		using is_transparent = void;

		template<typename T1, typename T2>
		constexpr bool operator()(const T1& l, const T2& r) const noexcept;
	};

	using map_type = std::multimap<std::vector<std::uint32_t>, range_type, comparer>;

public:
	explicit flat_distinguished_name(
		const security::pkcs7::signer_info_ref_pkcs7<range_type>& ref);
	explicit flat_distinguished_name(
		const security::pkcs7::signer_info_ref_cms<range_type>& ref);
	explicit flat_distinguished_name(
		const std::vector<asn1::crypto::relative_distinguished_name_type<range_type>>& dn)
	{
		build(dn);
	}

	template<std::size_t N>
	[[nodiscard]]
	std::optional<directory_string_type> get_directory_string(
		const std::array<std::uint32_t, N>& oid) const;

	template<std::size_t N>
	[[nodiscard]]
	std::optional<std::string> get_printable_string(
		const std::array<std::uint32_t, N>& oid) const;

	[[nodiscard]] bool empty() const noexcept
	{
		return parts_.empty();
	}

	[[nodiscard]] std::size_t size() const noexcept
	{
		return parts_.size();
	}

public:
	// Methods below return the first found attribute value.
	// Duplicates are allowed.
	[[nodiscard]]
	std::optional<directory_string_type> get_common_name() const;
	[[nodiscard]]
	std::optional<std::string> get_serial_number() const;
	[[nodiscard]]
	std::optional<std::string> get_country_name() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_locality_name() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_state_or_province_name() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_organization_name() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_organizational_unit_name() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_title() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_name() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_surname() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_given_name() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_initials() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_generation_qualifier() const;
	[[nodiscard]]
	std::optional<directory_string_type> get_pseudonim() const;

public:
	[[nodiscard]]
	const map_type& get_map() const noexcept
	{
		return parts_;
	}

private:
	void build(const std::vector<asn1::crypto::relative_distinguished_name_type<range_type>>& dn);

private:
	map_type parts_;
};

} //namespace pe_bliss::security::x500

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::x500::distinguished_name_errc> : true_type {};
} //namespace std
