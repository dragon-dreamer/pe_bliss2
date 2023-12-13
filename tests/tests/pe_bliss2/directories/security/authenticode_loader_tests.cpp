#include "pe_bliss2/security/authenticode_loader.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"

#include "buffers/input_container_buffer.h"
#include "buffers/input_memory_buffer.h"

#include "pe_bliss2/detail/security/image_security_directory.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"

#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"

#include "tests/pe_bliss2/directories/security/common_authenticode_data.h"
#include "tests/pe_bliss2/directories/security/non_contiguous_buffer.h"
#include "tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::security;

namespace
{
template<typename T>
class AuthenticodeLoaderTest : public testing::Test
{
public:
	using range_type = T;

public:
	pkcs7::attribute_map<range_type> map;
};

template<typename TargetRangeType, typename SourceRangeType>
struct RangeTypes
{
	using target_range_type = TargetRangeType;
	using source_range_type = SourceRangeType;
};

template<typename T>
class NestedAuthenticodeLoaderTest : public testing::Test
{
public:
	using target_range_type = typename T::target_range_type;
	using source_range_type = typename T::source_range_type;

public:
	pkcs7::attribute_map<source_range_type> map;
};

using tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;

using tested_nested_range_types = ::testing::Types<
	RangeTypes<vector_range_type, span_range_type>,
	RangeTypes<vector_range_type, vector_range_type>,
	RangeTypes<span_range_type, span_range_type>,
	RangeTypes<span_range_type, vector_range_type>>;
} //namespace

TYPED_TEST_SUITE(AuthenticodeLoaderTest, tested_range_types);
TYPED_TEST_SUITE(NestedAuthenticodeLoaderTest, tested_nested_range_types);

TYPED_TEST(AuthenticodeLoaderTest, ContiguousBufferInvalid)
{
	using range_type = typename TestFixture::range_type;

	buffers::input_container_buffer contiguous_buf;
	contiguous_buf.get_container() = { std::byte{1} };
	expect_throw_pe_error([&] {
		(void)load_authenticode_signature<range_type>(contiguous_buf);
	}, authenticode_loader_errc::invalid_authenticode_asn1_der);
}

TYPED_TEST(AuthenticodeLoaderTest, NonContiguousBufferInvalid)
{
	using range_type = typename TestFixture::range_type;

	non_contiguous_buffer non_contiguous_buf;
	non_contiguous_buf.get_container() = { std::byte{1} };

	if constexpr (std::is_same_v<range_type, span_range_type>)
	{
		expect_throw_pe_error([&] {
			(void)load_authenticode_signature<range_type>(non_contiguous_buf);
		}, authenticode_loader_errc::buffer_is_not_contiguous);
	}
	else
	{
		expect_throw_pe_error([&] {
			(void)load_authenticode_signature<range_type>(non_contiguous_buf);
		}, authenticode_loader_errc::invalid_authenticode_asn1_der);
	}
}

TYPED_TEST(AuthenticodeLoaderTest, Valid)
{
	using range_type = typename TestFixture::range_type;

	buffers::input_memory_buffer buf{
		reinterpret_cast<const std::byte*>(valid_authenticode.data()),
		valid_authenticode.size() };

	const auto result = load_authenticode_signature<range_type>(buf);
	ASSERT_EQ(result.get_signer_count(), 1u);
	ASSERT_EQ(result.get_signer(0u).get_digest_algorithm(), digest_algorithm::sha256);
}

TYPED_TEST(AuthenticodeLoaderTest, ValidWithTrailingGarbage)
{
	using range_type = typename TestFixture::range_type;

	buffers::input_container_buffer buf;
	buf.get_container().assign(
		reinterpret_cast<const std::byte*>(valid_authenticode.data()),
		reinterpret_cast<const std::byte*>(valid_authenticode.data())
			+ valid_authenticode.size());
	buf.get_container().emplace_back(std::byte{ 1u });

	expect_throw_pe_error([&] {
		(void)load_authenticode_signature<range_type>(buf);
	}, authenticode_loader_errc::invalid_authenticode_asn1_der);
}

TYPED_TEST(AuthenticodeLoaderTest, ValidCertificateType)
{
	using range_type = typename TestFixture::range_type;

	buffers::input_memory_buffer buf{
		reinterpret_cast<const std::byte*>(valid_authenticode.data()),
		valid_authenticode.size() };

	const auto result = load_authenticode_signature<range_type>(buf,
		pe_bliss::detail::security::win_certificate {
			.certificate_type
				= pe_bliss::detail::security::win_cert_type_pkcs_signed_data
		});

	ASSERT_EQ(result.get_signer_count(), 1u);
	ASSERT_EQ(result.get_signer(0u).get_digest_algorithm(), digest_algorithm::sha256);
}

TYPED_TEST(AuthenticodeLoaderTest, UnsupportedCertificateType)
{
	using range_type = typename TestFixture::range_type;

	buffers::input_memory_buffer buf{
		reinterpret_cast<const std::byte*>(valid_authenticode.data()),
		valid_authenticode.size() };

	expect_throw_pe_error([&] {
		(void)load_authenticode_signature<range_type>(buf,
		pe_bliss::detail::security::win_certificate{
			.certificate_type
				= pe_bliss::detail::security::win_cert_type_reserved_1
			});
	}, authenticode_loader_errc::unsupported_certificate_type);
}

TYPED_TEST(NestedAuthenticodeLoaderTest, LoadNestedSignaturesEmpty)
{
	using target_range_type = typename TestFixture::target_range_type;
	auto result = load_nested_signatures<target_range_type>(this->map);
	ASSERT_TRUE(result.empty());
}

TYPED_TEST(NestedAuthenticodeLoaderTest, LoadNestedSignaturesValid)
{
	using target_range_type = typename TestFixture::target_range_type;

	const std::vector<std::uint32_t> oid_nested_signature_attribute(
		asn1::crypto::pkcs7::authenticode::oid_nested_signature_attribute.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_nested_signature_attribute.cend());
	const std::vector<std::byte> valid_authenticode_copy{
		reinterpret_cast<const std::byte*>(valid_authenticode.data()),
		reinterpret_cast<const std::byte*>(valid_authenticode.data())
			+ valid_authenticode.size()
	};
	const std::vector<typename TestFixture::source_range_type> attr_data{
		valid_authenticode_copy, valid_authenticode_copy };
	this->map.get_map().try_emplace(oid_nested_signature_attribute, attr_data);

	auto result = load_nested_signatures<target_range_type>(this->map);
	ASSERT_EQ(result.size(), 2u);
	ASSERT_EQ(result[0].get_signer_count(), 1u);
	ASSERT_EQ(result[0].get_signer(0u).get_digest_algorithm(), digest_algorithm::sha256);
	ASSERT_EQ(result[1].get_signer_count(), 1u);
	ASSERT_EQ(result[1].get_signer(0u).get_digest_algorithm(), digest_algorithm::sha256);
}

TYPED_TEST(NestedAuthenticodeLoaderTest, LoadNestedSignaturesInvalid)
{
	using target_range_type = typename TestFixture::target_range_type;

	const std::vector<std::uint32_t> oid_nested_signature_attribute(
		asn1::crypto::pkcs7::authenticode::oid_nested_signature_attribute.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_nested_signature_attribute.cend());
	const std::vector<std::byte> valid_authenticode_copy{
		reinterpret_cast<const std::byte*>(valid_authenticode.data()),
		reinterpret_cast<const std::byte*>(valid_authenticode.data())
			+ valid_authenticode.size()
	};
	const std::vector<std::byte> invalid_authenticode{ std::byte{1} };
	const std::vector<typename TestFixture::source_range_type> attr_data{
		valid_authenticode_copy, invalid_authenticode };
	this->map.get_map().try_emplace(oid_nested_signature_attribute, attr_data);

	expect_throw_pe_error([this] {
		(void)load_nested_signatures<target_range_type>(this->map);
	}, authenticode_loader_errc::invalid_authenticode_asn1_der);
}
