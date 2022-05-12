#include "gtest/gtest.h"

#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>

#include "pe_bliss2/error_list.h"

TEST(ErrorListTests, ErrorListTest1)
{
	pe_bliss::error_list errors;
	EXPECT_FALSE(errors.has_errors());
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out)));
	EXPECT_TRUE(errors.has_errors());
	EXPECT_EQ(errors.get_errors(), pe_bliss::error_list::error_list_type{
		{ std::make_error_code(std::errc::timed_out) } });

	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::address_in_use)));
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out)));
	EXPECT_EQ(errors.get_errors(), (pe_bliss::error_list::error_list_type{
		pe_bliss::error_list::error_info{ std::make_error_code(std::errc::timed_out) },
		pe_bliss::error_list::error_info{ std::make_error_code(std::errc::address_in_use) } }));

	EXPECT_TRUE(errors.has_error(std::errc::timed_out));
	EXPECT_TRUE(errors.has_error(std::errc::address_in_use));
	EXPECT_FALSE(errors.has_error(std::errc::address_family_not_supported));

	EXPECT_NO_THROW(errors.clear_errors());
	EXPECT_FALSE(errors.has_errors());
}

TEST(ErrorListTests, ErrorListTest2)
{
	pe_bliss::error_list errors;
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out),
		"test"));
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out),
		"test"));
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::address_in_use),
		"test"));
	EXPECT_EQ(errors.get_errors(), (pe_bliss::error_list::error_list_type{
		{ std::make_error_code(std::errc::timed_out), "test" },
		{ std::make_error_code(std::errc::address_in_use), "test" } }));

	EXPECT_FALSE(errors.has_error(std::errc::timed_out));
	EXPECT_FALSE(errors.has_error(std::errc::address_in_use));
	EXPECT_TRUE(errors.has_error(std::errc::timed_out, "test"));
	EXPECT_TRUE(errors.has_error(std::errc::address_in_use, "test"));

	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::already_connected),
		123u));
	EXPECT_TRUE(errors.has_error(std::errc::already_connected, 123u));
	EXPECT_FALSE(errors.has_error(std::errc::already_connected, "test"));
	EXPECT_FALSE(errors.has_error(std::errc::already_connected, 456u));

	EXPECT_TRUE(errors.has_any_error(std::errc::timed_out));
	EXPECT_TRUE(errors.has_any_error(std::errc::address_in_use));
	EXPECT_FALSE(errors.has_any_error(std::errc::address_family_not_supported));
	EXPECT_TRUE(errors.has_any_error(std::errc::already_connected));
}

TEST(ErrorListTests, ErrorListTest3)
{
	pe_bliss::error_list errors;
	try
	{
		throw std::runtime_error("test");
	}
	catch (...)
	{
		EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out)));
	}

	try
	{
		EXPECT_NE(errors.get_errors().at(0).error, std::exception_ptr());
		std::rethrow_exception(errors.get_errors().at(0).error);
	}
	catch (const std::exception& e)
	{
		EXPECT_EQ(std::string(e.what()), "test");
	}
}
