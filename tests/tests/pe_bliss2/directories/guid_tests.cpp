#include "gtest/gtest.h"

#include "pe_bliss2/resources/guid.h"
#include "tests/tests/pe_bliss2/pe_error_helper.h"

using namespace pe_bliss::resources;

namespace
{
constexpr guid expected_guid{ 0x12345678u, 0xabcdu, 0xef12u,
	0x11u, 0x22u, 0x33u, 0x44u, 0x55u, 0x66u, 0x77u, 0x88u };
} //namespace

TEST(GuidTests, Parse)
{
	expect_throw_pe_error([] {
		(void)parse_guid("");
	}, guid_errc::invalid_guid);

	expect_throw_pe_error([] {
		(void)parse_guid("0");
	}, guid_errc::invalid_guid);

	expect_throw_pe_error([] {
		(void)parse_guid("00000000-0000-0000-0000-000000000000");
	}, guid_errc::invalid_guid);

	expect_throw_pe_error([] {
		(void)parse_guid("{00000000-0000-0000-0000-000000000000}", false);
	}, guid_errc::invalid_guid);

	expect_throw_pe_error([] {
		(void)parse_guid("{00000000-0000-0000-0000-00000g000000}");
	}, guid_errc::invalid_guid);

	expect_throw_pe_error([] {
		(void)parse_guid("{00000000-0000-0000-0000000000000000}");
	}, guid_errc::invalid_guid);

	expect_throw_pe_error([] {
		(void)parse_guid("{00000000-0000-0000-0000000000000000-}");
	}, guid_errc::invalid_guid);

	EXPECT_EQ(parse_guid("{12345678-abcd-ef12-1122-334455667788}"), expected_guid);
	EXPECT_EQ(parse_guid("12345678-abcd-ef12-1122-334455667788", false), expected_guid);
	EXPECT_EQ(parse_guid("{12345678-AbcD-ef12-1122-334455667788}"), expected_guid);
}

TEST(GuidTests, Format)
{
	EXPECT_EQ(to_string(expected_guid), "{12345678-ABCD-EF12-1122-334455667788}");
	EXPECT_EQ(to_string(expected_guid, false), "12345678-ABCD-EF12-1122-334455667788");
}
