#include "pe_bliss2/security/authenticode_program_info.h"

#include <variant>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/security/byte_range_types.h"
#include "pe_bliss2/security/pkcs7/attribute_map.h"

#include "simple_asn1/crypto/pkcs7/authenticode/oids.h"
#include "simple_asn1/crypto/pkcs7/authenticode/types.h"

using namespace pe_bliss::security;
using namespace pe_bliss::security::pkcs7;

namespace
{
template<typename T>
class ProgramInfoTest : public testing::Test
{
public:
	using range_type = T;
	using attribute_map_type = attribute_map<range_type>;
	using authenticode_program_info_type
		= authenticode_program_info<range_type>;

public:
	static const inline std::vector<std::uint32_t> oid_spc_sp_opus_info{
		asn1::crypto::pkcs7::authenticode::oid_spc_sp_opus_info.cbegin(),
		asn1::crypto::pkcs7::authenticode::oid_spc_sp_opus_info.cend() };

public:
	attribute_map_type map;
	authenticode_program_info_type info;
};

using tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;
} //namespace

TYPED_TEST_SUITE(ProgramInfoTest, tested_range_types);

TYPED_TEST(ProgramInfoTest, GetProgramNameEmpty)
{
	ASSERT_TRUE(std::holds_alternative<std::monostate>(
		this->info.get_program_name()));
}

TYPED_TEST(ProgramInfoTest, GetProgramNameString)
{
	this->info.get_underlying_info().program_name = std::string("test");
	auto variant = this->info.get_program_name();
	const auto* name = std::get_if<std::reference_wrapper<const std::string>>(&variant);
	ASSERT_NE(name, nullptr);
	ASSERT_EQ(name->get(), std::string("test"));
}

TYPED_TEST(ProgramInfoTest, GetProgramNameU16String)
{
	this->info.get_underlying_info().program_name = std::u16string(u"test");
	auto variant = this->info.get_program_name();
	const auto* name = std::get_if<std::reference_wrapper<const std::u16string>>(&variant);
	ASSERT_NE(name, nullptr);
	ASSERT_EQ(name->get(), std::u16string(u"test"));
}

TYPED_TEST(ProgramInfoTest, GetMoreInfoUrlEmpty)
{
	ASSERT_TRUE(std::holds_alternative<std::monostate>(
		this->info.get_more_info_url()));
}

TYPED_TEST(ProgramInfoTest, GetMoreInfoUrlString)
{
	this->info.get_underlying_info().more_info = std::string("test");
	auto variant = this->info.get_more_info_url();
	const auto* name = std::get_if<std::reference_wrapper<const std::string>>(&variant);
	ASSERT_NE(name, nullptr);
	ASSERT_EQ(name->get(), std::string("test"));
}

TYPED_TEST(ProgramInfoTest, GetMoreInfoUrlSpcString)
{
	this->info.get_underlying_info().more_info
		= asn1::crypto::pkcs7::authenticode::spc_string_type{ std::string("test") };
	auto variant = this->info.get_more_info_url();
	const auto* name = std::get_if<std::reference_wrapper<const std::string>>(&variant);
	ASSERT_NE(name, nullptr);
	ASSERT_EQ(name->get(), std::string("test"));
}

TYPED_TEST(ProgramInfoTest, GetMoreInfoUrlSpcU16String)
{
	this->info.get_underlying_info().more_info
		= asn1::crypto::pkcs7::authenticode::spc_string_type{ std::u16string(u"test") };
	auto variant = this->info.get_more_info_url();
	const auto* name = std::get_if<std::reference_wrapper<const std::u16string>>(&variant);
	ASSERT_NE(name, nullptr);
	ASSERT_EQ(name->get(), std::u16string(u"test"));
}

TYPED_TEST(ProgramInfoTest, GetMoreInfoUrlSpcSerializedObject)
{
	this->info.get_underlying_info().more_info
		= asn1::crypto::pkcs7::authenticode::spc_serialized_object<
			typename TestFixture::range_type>{};
	ASSERT_TRUE(std::holds_alternative<std::monostate>(
		this->info.get_more_info_url()));
}

TYPED_TEST(ProgramInfoTest, GetProgramInfoAbsent)
{
	ASSERT_FALSE(get_program_info(this->map));
}

TYPED_TEST(ProgramInfoTest, GetProgramInfoInvalid)
{
	const std::vector<std::byte> single_opus_info{ std::byte{1} };
	const std::vector<typename TestFixture::range_type> opus_info{ single_opus_info };
	this->map.get_map().try_emplace(TestFixture::oid_spc_sp_opus_info, opus_info);

	ASSERT_THROW((void)get_program_info(this->map), pe_bliss::pe_error);
}

TYPED_TEST(ProgramInfoTest, GetProgramInfoValid)
{
	const std::vector<std::byte> single_opus_info{
		std::byte{0x30u}, //SEQUENCE
		std::byte{7}, //sequence length

		std::byte{0xa1u}, //TAGGED (moreInfo spc_link)
		std::byte{5}, //tagged length

		std::byte{0x80u}, //TAGGED (ia5_string)
		std::byte{3}, //ia5_string length
		std::byte{'a'}, std::byte{'b'}, std::byte{'c'},

		std::byte{0}, std::byte{0}, //trailing garbage: zero bytes are OK
	};
	const std::vector<typename TestFixture::range_type> opus_info{ single_opus_info };
	this->map.get_map().try_emplace(TestFixture::oid_spc_sp_opus_info, opus_info);

	auto result = get_program_info(this->map);
	ASSERT_TRUE(result);
	auto variant = result->get_more_info_url();
	const auto* name = std::get_if<std::reference_wrapper<const std::string>>(&variant);
	ASSERT_NE(name, nullptr);
	ASSERT_EQ(name->get(), std::string("abc"));
}

TYPED_TEST(ProgramInfoTest, GetProgramInfoTrailingGarbage)
{
	const std::vector<std::byte> single_opus_info{
		std::byte{0x30u}, //SEQUENCE
		std::byte{7}, //sequence length

		std::byte{0xa1u}, //TAGGED (moreInfo spc_link)
		std::byte{5}, //tagged length

		std::byte{0x80u}, //TAGGED (ia5_string)
		std::byte{3}, //ia5_string length
		std::byte{'a'}, std::byte{'b'}, std::byte{'c'},

		std::byte{0}, std::byte{1}, //trailing garbage
	};
	const std::vector<typename TestFixture::range_type> opus_info{ single_opus_info };
	this->map.get_map().try_emplace(TestFixture::oid_spc_sp_opus_info, opus_info);

	ASSERT_THROW((void)get_program_info(this->map), pe_bliss::pe_error);
}
