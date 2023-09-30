#include "pe_bliss2/security/authenticode_pkcs7.h"

#include <cstddef>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/security/byte_range_types.h"

using namespace pe_bliss::security;
using namespace pe_bliss::security::pkcs7;

namespace
{
template<typename T>
class AuthenticodePkcs7Test : public testing::Test
{
public:
	using range_type = T;
	using authenticode_pkcs7_type = authenticode_pkcs7<range_type>;

public:
	authenticode_pkcs7_type message{};
};

using tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;
} //namespace

TYPED_TEST_SUITE(AuthenticodePkcs7Test, tested_range_types);

TYPED_TEST(AuthenticodePkcs7Test, GetImageHash)
{
	const std::vector<std::byte> data{ std::byte{1}, std::byte{2} };
	this->message.get_content_info()
		.data.content_info.content.digest.value.digest = data;

	ASSERT_EQ(this->message.get_image_hash().data(),
		this->message.get_content_info()
		.data.content_info.content.digest.value.digest.data());
}
