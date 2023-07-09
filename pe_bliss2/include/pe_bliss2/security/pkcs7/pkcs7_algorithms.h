#pragma once

namespace pe_bliss::security::pkcs7
{

enum class digest_algorithm
{
	md5,
	sha1,
	sha256,
	unknown
};

enum class digest_encryption_algorithm
{
	rsa,
	dsa,
	unknown
};

} //namespace pe_bliss::security::pkcs7
