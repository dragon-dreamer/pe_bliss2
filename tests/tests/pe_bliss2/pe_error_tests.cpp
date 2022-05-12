#include <ios>
#include <system_error>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"

TEST(PeErrorTests, PeErrorTest)
{
	try
	{
		throw pe_bliss::pe_error(std::make_error_code(std::errc::already_connected));
	}
	catch (const pe_bliss::pe_error& e)
	{
		EXPECT_EQ(e.code(), std::errc::already_connected);
	}
}

TEST(PeErrorTests, PeErrorWrapperTest1)
{
	pe_bliss::pe_error_wrapper wrapper;
	EXPECT_NO_THROW(wrapper.throw_on_error());
	EXPECT_EQ(static_cast<std::error_code>(wrapper), std::error_code{});
	EXPECT_FALSE(static_cast<bool>(wrapper));
}

TEST(PeErrorTests, PeErrorWrapperTest2)
{
	pe_bliss::pe_error_wrapper wrapper(std::io_errc::stream);
	EXPECT_THROW(wrapper.throw_on_error(), pe_bliss::pe_error);
	EXPECT_EQ(static_cast<std::error_code>(wrapper), std::io_errc::stream);
	EXPECT_TRUE(static_cast<bool>(wrapper));
}
