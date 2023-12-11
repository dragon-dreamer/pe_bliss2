#include "pe_bliss2/security/x509/x509_certificate_store.h"

#include <algorithm>
#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/x509/x509_certificate.h"

#include "simple_asn1/crypto/algorithms.h"
#include "simple_asn1/crypto/x509/types.h"

using namespace pe_bliss::security;
using namespace pe_bliss::security::x509;

namespace
{
template<typename T>
class X509CertificateStoreTests : public testing::Test
{
public:
	using range_type = T;
	using cert_type = x509_certificate<range_type>;
};

using tested_types = ::testing::Types<vector_range_type, span_range_type>;
} //namespace

TYPED_TEST_SUITE(X509CertificateStoreTests, tested_types);

TYPED_TEST(X509CertificateStoreTests, Empty)
{
	using cert_type = typename TestFixture::cert_type;
	x509_certificate_store<cert_type> store;
	store.reserve(10);
	EXPECT_EQ(store.find_certificate({}, {}), nullptr);
}

TYPED_TEST(X509CertificateStoreTests, AddFind)
{
	using range_type = typename TestFixture::range_type;
	using cert_type = typename TestFixture::cert_type;
	x509_certificate_store<cert_type> store;

	std::vector<std::byte> issuer1{std::byte{1}, std::byte{2}};
	std::vector<std::byte> sn1{std::byte{3}, std::byte{4}};
	asn1::crypto::x509::certificate<range_type> cert1;
	cert1.tbs_cert.issuer.raw = issuer1;
	cert1.tbs_cert.serial_number = sn1;
	cert_type cert1_ref(cert1);
	cert_type cert1_ref_copy(cert1);

	std::vector<std::byte> issuer2{std::byte{5}, std::byte{6}};
	std::vector<std::byte> sn2{std::byte{4}, std::byte{2}};
	asn1::crypto::x509::certificate<range_type> cert2;
	cert2.tbs_cert.issuer.raw = issuer2;
	cert2.tbs_cert.serial_number = sn2;
	cert_type cert2_ref(cert2);

	ASSERT_TRUE(store.add_certificate(std::move(cert1_ref)));
	ASSERT_FALSE(store.add_certificate(std::move(cert1_ref_copy)));
	ASSERT_TRUE(store.add_certificate(std::move(cert2_ref)));

	const auto* found1 = store.find_certificate(sn1, issuer1);
	ASSERT_NE(found1, nullptr);
	ASSERT_TRUE(std::ranges::equal(found1->get_raw_data().tbs_cert.issuer.raw,
		cert1.tbs_cert.issuer.raw));
	ASSERT_TRUE(std::ranges::equal(found1->get_raw_data().tbs_cert.serial_number,
		cert1.tbs_cert.serial_number));
	const auto* found2 = store.find_certificate(sn2, issuer2);
	ASSERT_NE(found2, nullptr);
	ASSERT_TRUE(std::ranges::equal(found2->get_raw_data().tbs_cert.issuer.raw,
		cert2.tbs_cert.issuer.raw));
	ASSERT_TRUE(std::ranges::equal(found2->get_raw_data().tbs_cert.serial_number,
		cert2.tbs_cert.serial_number));

	ASSERT_EQ(store.find_certificate(sn2, issuer1), nullptr);
	ASSERT_EQ(store.find_certificate(sn1, issuer2), nullptr);
}
