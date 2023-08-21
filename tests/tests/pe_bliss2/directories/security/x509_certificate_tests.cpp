#include "pe_bliss2/security/x509/x509_certificate.h"

#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

#include "simple_asn1/crypto/algorithms.h"
#include "simple_asn1/crypto/x509/types.h"

using namespace pe_bliss::security;
using namespace pe_bliss::security::x509;

TEST(X509CertificateTests, X509CertificateRef)
{
	using range_type = std::vector<std::byte>;
	asn1::crypto::x509::certificate<range_type> cert{};

	x509_certificate_ref<range_type> ref(cert);

	range_type serial_number{ std::byte{1}, std::byte{2} };
	cert.tbs_cert.serial_number = serial_number;
	ASSERT_EQ(ref.get_serial_number(), serial_number);

	range_type issuer{ std::byte{3}, std::byte{4} };
	cert.tbs_cert.issuer.raw = issuer;
	ASSERT_EQ(ref.get_raw_issuer(), issuer);

	range_type public_key{ std::byte{5}, std::byte{6} };
	cert.tbs_cert.pki.subject_publickey.container = public_key;
	ASSERT_EQ(ref.get_public_key(), public_key);

	ASSERT_FALSE(ref.get_signature_algorithm_parameters().has_value());
	range_type signature_parameters{ std::byte{7}, std::byte{8} };
	cert.tbs_cert.pki.algorithm.parameters = signature_parameters;
	ASSERT_EQ(ref.get_signature_algorithm_parameters(), signature_parameters);
	ASSERT_EQ(ref.get_raw_public_key_algorithm().parameters, signature_parameters);

	ASSERT_EQ(ref.get_public_key_algorithm().encryption_alg, digest_encryption_algorithm::unknown);
	ASSERT_FALSE(ref.get_public_key_algorithm().hash_alg.has_value());
	cert.tbs_cert.pki.algorithm.algorithm.container = {
		asn1::crypto::pki::id_sha1_with_rsa.begin(),
		asn1::crypto::pki::id_sha1_with_rsa.end() };
	ASSERT_EQ(ref.get_public_key_algorithm().encryption_alg, digest_encryption_algorithm::rsa);
	ASSERT_EQ(ref.get_public_key_algorithm().hash_alg, digest_algorithm::sha1);
}
