#pragma once

#include <system_error>
#include <type_traits>
#include <utility>

#include "gtest/gtest.h"

#include "pe_bliss2/pe_error.h"

template<typename Func, typename ErrorCode>
	requires(std::is_error_code_enum_v<ErrorCode>)
void expect_throw_pe_error(Func&& func, ErrorCode ec)
{
	EXPECT_THROW({
		try
		{
			(void)std::forward<Func>(func)();
		}
		catch (const pe_bliss::pe_error& e)
		{
			EXPECT_EQ(e.code(), ec);
			throw;
		}
	}, pe_bliss::pe_error);
}
