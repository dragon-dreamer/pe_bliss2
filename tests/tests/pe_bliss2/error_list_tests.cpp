#include "gtest/gtest.h"

#include <system_error>

#include "pe_bliss2/error_list.h"

TEST(ErrorListTests, ErrorListTest)
{
	pe_bliss::error_list errors;
	EXPECT_FALSE(errors.has_errors());
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out)));
	EXPECT_TRUE(errors.has_errors());
	EXPECT_EQ(errors.get_errors(), pe_bliss::error_list::error_list_type{
		std::make_error_code(std::errc::timed_out) });

	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::address_in_use)));
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out)));
	EXPECT_EQ(errors.get_errors(), (pe_bliss::error_list::error_list_type{
		std::make_error_code(std::errc::timed_out),
		std::make_error_code(std::errc::address_in_use) }));
}
