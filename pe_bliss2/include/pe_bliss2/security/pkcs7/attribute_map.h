#pragma once

#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <span>
#include <system_error>
#include <unordered_map>
#include <utility>
#include <vector>

#include "pe_bliss2/security/byte_range_types.h"

#include "utilities/range_helpers.h"

namespace pe_bliss::security::pkcs7
{

enum class attribute_map_errc
{
	absent_attribute_value,
	multiple_attribute_values
};

std::error_code make_error_code(attribute_map_errc) noexcept;

template<typename RangeType>
using attribute_map_type = std::unordered_map<
	std::span<const std::uint32_t>,
	std::reference_wrapper<const std::vector<RangeType>>,
	utilities::range_hash, utilities::range_equal
>;

template<typename RangeType>
class attribute_map
{
public:
	[[nodiscard]]
	std::optional<span_range_type> get_attribute(
		std::span<const std::uint32_t> oid) const;

	[[nodiscard]]
	std::vector<span_range_type> get_attributes(
		std::span<const std::uint32_t> oid) const;

	[[nodiscard]]
	std::optional<span_range_type> get_message_digest() const;
	[[nodiscard]]
	std::optional<span_range_type> get_content_type() const;
	[[nodiscard]]
	std::optional<span_range_type> get_signing_time() const;

public:
	[[nodiscard]]
	attribute_map_type<RangeType>& get_map() & noexcept
	{
		return map_;
	}

	[[nodiscard]]
	const attribute_map_type<RangeType>& get_map() const& noexcept
	{
		return map_;
	}

	[[nodiscard]]
	attribute_map_type<RangeType> get_map() && noexcept
	{
		return std::move(map_);
	}

private:
	attribute_map_type<RangeType> map_;
};

namespace impl
{
[[nodiscard]]
span_range_type decode_octet_string(span_range_type source);
} //namespace impl

template<typename RangeType>
[[nodiscard]]
bool verify_message_digest_attribute(std::span<const std::byte> calculated_message_digest,
	const attribute_map<RangeType>& signer_authenticated_attributes)
{
	auto message_digest = signer_authenticated_attributes.get_message_digest();
	if (!message_digest)
		return false;
	return std::ranges::equal(
		impl::decode_octet_string(*message_digest),
		calculated_message_digest);
}

} //namespace pe_bliss::security::pkcs7

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::pkcs7::attribute_map_errc> : true_type {};
} //namespace std
