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

#include "pe_bliss2/security/pkcs7/pkcs7_algorithms.h"

#include "simple_asn1/crypto/pkcs7/types.h"

#include "utilities/hash.h"

namespace pe_bliss::security::pkcs7
{

enum class pkcs7_errc
{
	duplicate_attribute_oid,
	absent_attribute_value,
	multiple_attribute_values
};

std::error_code make_error_code(pkcs7_errc) noexcept;

using span_range_type = std::span<const std::byte>;
using vector_range_type = std::vector<std::byte>;

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
using attribute_map_type = std::unordered_map<
	std::span<const std::uint32_t>, std::reference_wrapper<const std::vector<RangeType>>,
	impl::range_hash, impl::range_equal
>;

template<typename RangeType>
class attribute_map
{
public:
	[[nodiscard]]
	std::optional<span_range_type> get_attribute(
		std::span<const std::uint32_t> oid) const;

	[[nodiscard]]
	std::optional<span_range_type> get_message_digest() const;
	[[nodiscard]]
	std::optional<span_range_type> get_content_type() const;

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

template<typename RangeType>
class [[nodiscard]] signer_info
{
public:
	using signer_info_type = asn1::crypto::pkcs7::signer_info<RangeType>;

	constexpr signer_info(const signer_info_type& signer_info_ref) noexcept
		: signer_info_ref_(signer_info_ref)
	{
	}

public:
	[[nodiscard]]
	digest_algorithm get_digest_algorithm() const noexcept;
	[[nodiscard]]
	digest_encryption_algorithm get_digest_encryption_algorithm() const noexcept;

	[[nodiscard]]
	attribute_map<RangeType> get_authenticated_attributes() const;

private:
	const signer_info_type& signer_info_ref_;
};

template<typename RangeType, typename ContentInfo>
class [[nodiscard]] pkcs7
{
public:
	using content_info_type = ContentInfo;

public:
	[[nodiscard]]
	std::size_t get_signer_count() const noexcept
	{
		return content_info_.data.signer_infos.size();
	}

	[[nodiscard]]
	signer_info<RangeType> get_signer(std::size_t index) const
	{
		return content_info_.data.signer_infos.at(index);
	}

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

} //namespace pe_bliss::security::pkcs7

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::security::pkcs7::pkcs7_errc> : true_type {};
} //namespace std
