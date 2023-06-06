#include "gtest/gtest.h"

#include "pe_bliss2/tls/tls_directory.h"

#include "tests/pe_bliss2/pe_error_helper.h"

TEST(TlsDirectoryTests, Alignment)
{
	pe_bliss::tls::tls_directory32 dir;
	EXPECT_EQ(dir.get_max_type_alignment(), 0u);

	expect_throw_pe_error([&dir] {
		dir.set_max_type_alignment(0x123u);
	}, pe_bliss::tls::tls_directory_errc::invalid_alignment_value);

	expect_throw_pe_error([&dir] {
		dir.set_max_type_alignment(0xffffffu);
	}, pe_bliss::tls::tls_directory_errc::too_big_alignment_value);

	EXPECT_EQ(dir.get_max_type_alignment(), 0u);
	EXPECT_EQ(dir.get_descriptor()->characteristics, 0u);

	EXPECT_NO_THROW(dir.set_max_type_alignment(32u));

	EXPECT_EQ(dir.get_max_type_alignment(), 32u);
	EXPECT_EQ(dir.get_descriptor()->characteristics, 0x600000u);
}
