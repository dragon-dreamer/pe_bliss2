#include "pe_bliss2/security/authenticode_certificate_store.h"

#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"

#include "tests/pe_bliss2/pe_error_helper.h"

#include "simple_asn1/crypto/x509/types.h"

using namespace pe_bliss::security;

namespace
{
template<typename T>
class AuthenticodeCertificateStoreTests : public testing::Test
{
public:
	using signature_type = T;

	static inline const std::vector serial_number1{
		std::byte{1}, std::byte{2}
	};
	static inline const std::vector serial_number2{
		std::byte{1}, std::byte{2}, std::byte{3}
	};

	static inline const std::vector issuer1{
		std::byte{5}, std::byte{6}
	};
	static inline const std::vector issuer2{
		std::byte{5}, std::byte{6}, std::byte{7}
	};
};

using tested_range_types = ::testing::Types<
	authenticode_pkcs7<vector_range_type>,
	authenticode_pkcs7<span_range_type>,
	authenticode_signature_cms_info_ms_bug_workaround_type<vector_range_type>,
	authenticode_signature_cms_info_ms_bug_workaround_type<span_range_type>,
	authenticode_signature_cms_info_type<vector_range_type>,
	authenticode_signature_cms_info_type<span_range_type>>;
} //namespace

TYPED_TEST_SUITE(AuthenticodeCertificateStoreTests, tested_range_types);

TYPED_TEST(AuthenticodeCertificateStoreTests, Empty1)
{
	using signature_type = typename TestFixture::signature_type;
	signature_type signature{};

	pe_bliss::error_list errors;
	const auto certs = build_certificate_store(signature, &errors);
	ASSERT_TRUE(certs.empty());
	expect_contains_errors(errors,
		certificate_store_errc::absent_certificates);
}

TYPED_TEST(AuthenticodeCertificateStoreTests, Empty2)
{
	using signature_type = typename TestFixture::signature_type;
	signature_type signature{};
	signature.get_content_info().data.certificates.emplace();

	pe_bliss::error_list errors;
	const auto certs = build_certificate_store(signature, &errors);
	ASSERT_TRUE(certs.empty());
	expect_contains_errors(errors,
		certificate_store_errc::absent_certificates);
}

TYPED_TEST(AuthenticodeCertificateStoreTests, WithCerts)
{
	using signature_type = typename TestFixture::signature_type;
	signature_type signature{};

	auto& certificates = signature.get_content_info().data.certificates.emplace();
	{
		auto& cert = certificates.emplace_back().template emplace<0>();
		cert.tbs_cert.serial_number = this->serial_number1;
		cert.tbs_cert.issuer.raw = this->issuer1;
	}
	{
		auto& cert = certificates.emplace_back().template emplace<0>();
		cert.tbs_cert.serial_number = this->serial_number2;
		cert.tbs_cert.issuer.raw = this->issuer2;
	}

	pe_bliss::error_list errors;
	const auto certs = build_certificate_store(signature, &errors);
	ASSERT_FALSE(certs.empty());
	ASSERT_EQ(certs.size(), 2u);
	ASSERT_NE(certs.find_certificate(this->serial_number1, this->issuer1), nullptr);
	ASSERT_NE(certs.find_certificate(this->serial_number2, this->issuer2), nullptr);
	ASSERT_EQ(certs.find_certificate(this->serial_number2, this->issuer1), nullptr);
	expect_contains_errors(errors);
}

TYPED_TEST(AuthenticodeCertificateStoreTests, WithDuplicateCerts)
{
	using signature_type = typename TestFixture::signature_type;
	signature_type signature{};

	auto& certificates = signature.get_content_info().data.certificates.emplace();
	{
		auto& cert = certificates.emplace_back().template emplace<0>();
		cert.tbs_cert.serial_number = this->serial_number1;
		cert.tbs_cert.issuer.raw = this->issuer1;
	}
	{
		auto& cert = certificates.emplace_back().template emplace<0>();
		cert.tbs_cert.serial_number = this->serial_number1;
		cert.tbs_cert.issuer.raw = this->issuer1;
	}

	pe_bliss::error_list errors;
	const auto certs = build_certificate_store(signature, &errors);
	ASSERT_FALSE(certs.empty());
	ASSERT_EQ(certs.size(), 1u);
	ASSERT_NE(certs.find_certificate(this->serial_number1, this->issuer1), nullptr);
	ASSERT_EQ(certs.find_certificate(this->serial_number2, this->issuer2), nullptr);
	expect_contains_errors(errors,
		certificate_store_errc::duplicate_certificates);
}
