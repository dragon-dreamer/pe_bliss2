#pragma once

#include <system_error>
#include <type_traits>
#include <utility>

#include "gtest/gtest.h"

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/pe_error.h"

namespace impl
{

constexpr void expect_contains_error(const pe_bliss::error_list&)
{
}

template<typename ErrorCode, typename... ErrorCodes>
void expect_contains_error(const pe_bliss::error_list& errs,
	ErrorCode code, ErrorCodes... codes)
{
	EXPECT_TRUE(errs.has_any_error(code));
	expect_contains_error(errs, codes...);
}

} //namespace impl

template<typename... ErrorCode>
void expect_contains_errors(const pe_bliss::error_list& errs,
	ErrorCode... code)
{
	EXPECT_EQ(errs.get_errors().size(), sizeof...(code));
	impl::expect_contains_error(errs, code...);
}

template<typename ErrorCode>
void expect_contains_error(const pe_bliss::error_list& errs,
	ErrorCode code)
{
	impl::expect_contains_error(errs, code);
}

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
