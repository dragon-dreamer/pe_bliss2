#include "pe_bliss2/security/x500/flat_distinguished_name.h"

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"

using namespace pe_bliss::security;

namespace
{
template<typename T>
class FlatDistinguishedNameTests : public testing::Test
{
public:
	using range_type = T;
	using dn_type = x500::flat_distinguished_name<range_type>;

public:
	template<typename DN>
	void test(const DN& dn, bool with_duplicates = false) const
	{
		ASSERT_FALSE(dn.empty());
		ASSERT_EQ(dn.size(), 4u + with_duplicates);

		std::optional<std::string> country_name;
		ASSERT_NO_THROW(country_name = dn.get_country_name());
		ASSERT_EQ(country_name, "US");

		std::optional<typename DN::directory_string_type> state_name;
		ASSERT_NO_THROW(state_name = dn.get_state_or_province_name());
		ASSERT_TRUE(state_name);
		const auto* state_name_str = std::get_if<std::u16string>(&*state_name);
		ASSERT_NE(state_name_str, nullptr);
		ASSERT_EQ(*state_name_str, u"XYZ");

		std::optional<typename DN::directory_string_type> locality_name;
		ASSERT_NO_THROW(locality_name = dn.get_locality_name());
		ASSERT_TRUE(locality_name);
		const auto* locality_name_str = std::get_if<std::u32string>(&*locality_name);
		ASSERT_NE(locality_name_str, nullptr);
		ASSERT_EQ(*locality_name_str, U"ab");

		ASSERT_FALSE(dn.get_common_name());
		ASSERT_FALSE(dn.get_serial_number());

		ASSERT_THROW((void)dn.get_organization_name(), pe_bliss::pe_error);
	}

	template<typename Issuer>
	void fill(Issuer& issuer, bool add_duplicate = false) const
	{
		{
			auto& level = issuer.emplace_back();
			level.push_back({
				.attribute_type = asn1::crypto::object_identifier_type {
					.container = {2,5,4,6}
				},
				.attribute_value = this->raw_country_name
			});
		}

		if (add_duplicate)
		{
			auto& level = issuer.emplace_back();
			level.push_back({
				.attribute_type = asn1::crypto::object_identifier_type {
					.container = {2,5,4,10}
				}
			});
		}

		{
			auto& level = issuer.emplace_back();
			level.push_back({
				.attribute_type = asn1::crypto::object_identifier_type {
					.container = {2,5,4,8}
				},
				.attribute_value = this->raw_state_name
			});
			level.push_back({
				.attribute_type = asn1::crypto::object_identifier_type {
					.container = {2,5,4,7}
				},
				.attribute_value = this->raw_locality_name
			});
			level.push_back({
				.attribute_type = asn1::crypto::object_identifier_type {
					.container = {2,5,4,10}
				}
			});
		}
	}

public:
	const std::vector<std::byte> raw_country_name{
		std::byte{0x13u}, std::byte{0x02u}, std::byte{'U'}, std::byte{'S'} };
	const std::vector<std::byte> raw_state_name{
		std::byte{0x1eu}, std::byte{0x06u},
		std::byte{}, std::byte{'X'}, std::byte{}, std::byte{'Y'},
		std::byte{}, std::byte{'Z'} };
	const std::vector<std::byte> raw_locality_name{
		std::byte{0x1cu}, std::byte{0x08u},
		std::byte{}, std::byte{}, std::byte{}, std::byte{'a'},
		std::byte{}, std::byte{}, std::byte{}, std::byte{'b'} };
};

using tested_range_types = ::testing::Types<
	vector_range_type, span_range_type>;

} //namespace

TYPED_TEST_SUITE(FlatDistinguishedNameTests, tested_range_types);

TYPED_TEST(FlatDistinguishedNameTests, FromPkcs7)
{
	pkcs7::signer_info_pkcs7<typename TestFixture::range_type> info{};
	auto& issuer = info.get_underlying().issuer_and_sn.issuer.value;
	this->fill(issuer);

	const typename TestFixture::dn_type dn{ info };
	this->test(dn);
}

TYPED_TEST(FlatDistinguishedNameTests, FromCms)
{
	pkcs7::signer_info_cms<typename TestFixture::range_type> info{};
	auto& issuer = info.get_underlying().sid
		.template emplace<asn1::crypto::pkcs7::issuer_and_serial_number<
			typename TestFixture::range_type>>()
		.issuer.value;
	this->fill(issuer);

	const typename TestFixture::dn_type dn{ info };
	this->test(dn);
}

TYPED_TEST(FlatDistinguishedNameTests, FromCmsWithDuplicates)
{
	pkcs7::signer_info_cms<typename TestFixture::range_type> info{};
	auto& issuer = info.get_underlying().sid
		.template emplace<asn1::crypto::pkcs7::issuer_and_serial_number<
		typename TestFixture::range_type>>()
		.issuer.value;
	this->fill(issuer, true);

	const typename TestFixture::dn_type dn{ info };
	this->test(dn, true);
}
