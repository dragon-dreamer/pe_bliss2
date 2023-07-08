#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "simple_asn1/crypto/pkcs7/authenticode/types.h"

#include "pe_bliss2/security/authenticode_digest_algorithm.h"

#include "utilities/hash.h"

namespace pe_bliss::security
{

enum class authenticode_pkcs7_errc
{
	duplicate_attribute_oid,
	absent_attribute_value,
	multiple_attribute_values
};

std::error_code make_error_code(authenticode_pkcs7_errc) noexcept;

using authenticode_span_range_type = std::span<const std::byte>;
using authenticode_vector_range_type = std::vector<std::byte>;

enum class authenticode_digest_encryption_algorithm
{
	rsa,
	dsa,
	unknown
};

namespace impl
{
struct range_hash
{
	template<typename T>
	auto operator()(const T& range) const noexcept
	{
		std::size_t hash{};
		for (const auto& elem : range)
			utilities::hash_combine(hash, std::hash<typename T::value_type>{}(elem));

		return hash;
	}

	using is_transparent = void;
};

struct range_equal
{
	template<typename T1, typename T2>
	constexpr bool operator()(const T1& range_l, const T2& range_r) const noexcept
	{
		return std::ranges::equal(range_l, range_r);
	}

	using is_transparent = void;
};
} //namespace impl

template<typename RangeType>
using authenticode_attribute_map_type = std::unordered_map<
	std::span<const std::uint32_t>, std::reference_wrapper<const std::vector<RangeType>>,
	impl::range_hash, impl::range_equal
>;

template<typename RangeType>
class authenticode_attribute_map
{
public:
	[[nodiscard]]
	std::optional<authenticode_span_range_type> get_attribute(
		std::span<const std::uint32_t> oid) const;

	[[nodiscard]]
	std::optional<authenticode_span_range_type> get_message_digest() const;
	[[nodiscard]]
	std::optional<authenticode_span_range_type> get_content_type() const;
	[[nodiscard]]
	std::optional<authenticode_span_range_type> get_spc_sp_opus_info() const;

public:
	[[nodiscard]]
	authenticode_attribute_map_type<RangeType>& get_map() & noexcept
	{
		return map_;
	}

	[[nodiscard]]
	const authenticode_attribute_map_type<RangeType>& get_map() const& noexcept
	{
		return map_;
	}

	[[nodiscard]]
	authenticode_attribute_map_type<RangeType> get_map() && noexcept
	{
		return std::move(map_);
	}

private:
	authenticode_attribute_map_type<RangeType> map_;
};

template<typename RangeType>
class [[nodiscard]] authenticode_pkcs7
{
public:
	using content_info_type = asn1::crypto::pkcs7::authenticode::content_info<RangeType>;

public:
	[[nodiscard]]
	authenticode_digest_algorithm get_digest_algorithm() const noexcept;
	[[nodiscard]]
	authenticode_digest_encryption_algorithm get_digest_encryption_algorithm() const noexcept;
	[[nodiscard]]
	authenticode_span_range_type get_image_hash() const noexcept;

	[[nodiscard]]
	authenticode_attribute_map<RangeType> get_authenticated_attributes() const;

public:
	[[nodiscard]]
	const content_info_type& get_content_info() const noexcept
	{
		return content_info_;
	}

	[[nodiscard]]
	content_info_type& get_content_info() noexcept
	{
		return content_info_;
	}

private:
	content_info_type content_info_;
};

} //namespace pe_bliss::security

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::authenticode_pkcs7_errc> : true_type {};
} //namespace std
