#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>
#include <variant>

#include "pe_bliss2/error_list.h"

using namespace ::testing;

TEST(ErrorListTests, ErrorListTest1)
{
	pe_bliss::error_list errors;
	ASSERT_FALSE(errors.has_errors());
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out)));
	ASSERT_TRUE(errors.has_errors());
	ASSERT_EQ(errors.get_errors()->size(), 1u);
	ASSERT_EQ(errors.get_errors()->begin()->first.code,
		std::make_error_code(std::errc::timed_out));
	ASSERT_TRUE(std::holds_alternative<std::monostate>(
		errors.get_errors()->begin()->first.context));
	ASSERT_FALSE(errors.get_errors()->begin()->second.error);
	
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::address_in_use)));
	EXPECT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out)));
	ASSERT_THAT(*errors.get_errors(), UnorderedElementsAre(
		Pair(pe_bliss::error_list::error_context{ std::make_error_code(std::errc::timed_out) },
			pe_bliss::error_list::error_info{}),
		Pair(pe_bliss::error_list::error_context{ std::make_error_code(std::errc::address_in_use) },
			pe_bliss::error_list::error_info{})));

	EXPECT_TRUE(errors.has_error(std::errc::timed_out));
	EXPECT_TRUE(errors.has_error(std::errc::address_in_use));
	EXPECT_FALSE(errors.has_error(std::errc::address_family_not_supported));

	EXPECT_NO_THROW(errors.clear_errors());
	EXPECT_FALSE(errors.has_errors());
}

TEST(ErrorListTests, ErrorListTest2)
{
	pe_bliss::error_list errors;
	ASSERT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out),
		"test"));
	ASSERT_NO_THROW(errors.add_error(std::make_error_code(std::errc::timed_out),
		"test"));
	ASSERT_NO_THROW(errors.add_error(std::make_error_code(std::errc::address_in_use),
		"test"));
	ASSERT_TRUE(errors.has_errors());
	ASSERT_THAT(*errors.get_errors(), UnorderedElementsAre(
		Pair(pe_bliss::error_list::error_context{ std::make_error_code(std::errc::timed_out), "test" },
			pe_bliss::error_list::error_info{}),
		Pair(pe_bliss::error_list::error_context{ std::make_error_code(std::errc::address_in_use), "test" },
			pe_bliss::error_list::error_info{})));

	EXPECT_FALSE(errors.has_error(std::errc::timed_out));
	EXPECT_FALSE(errors.has_error(std::errc::address_in_use));
	EXPECT_TRUE(errors.has_error(std::errc::timed_out, "test"));
	EXPECT_TRUE(errors.has_error(std::errc::address_in_use, "test"));

	ASSERT_NO_THROW(errors.add_error(std::make_error_code(std::errc::already_connected),
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
		EXPECT_NE(errors.get_errors()->begin()->second.error, std::exception_ptr());
		std::rethrow_exception(errors.get_errors()->begin()->second.error);
	}
	catch (const std::exception& e)
	{
		EXPECT_EQ(std::string(e.what()), "test");
	}
}
