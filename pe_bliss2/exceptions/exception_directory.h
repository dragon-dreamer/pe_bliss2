#pragma once

#include <utility>
#include <variant>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/exceptions/x64/x64_exception_directory.h"
#include "pe_bliss2/exceptions/arm/arm_exception_directory.h"
#include "pe_bliss2/exceptions/arm64/arm64_exception_directory.h"

namespace pe_bliss::exceptions
{

template<typename... Bases>
class [[nodiscard]] exception_directory_base
	: public Bases...
{
public:
	using exception_directory_type = std::variant<
		x64::exception_directory_base<Bases...>,
		arm64::exception_directory_base<Bases...>,
		arm::exception_directory_base<Bases...>
	>;
	using exception_directory_list_type = std::vector<exception_directory_type>;

public:
	[[nodiscard]]
	exception_directory_list_type& get_directories() & noexcept
	{
		return directories_;
	}

	[[nodiscard]]
	const exception_directory_list_type& get_directories() const& noexcept
	{
		return directories_;
	}

	[[nodiscard]]
	exception_directory_list_type get_directories() && noexcept
	{
		return std::move(directories_);
	}

private:
	exception_directory_list_type directories_;
};

using exception_directory = exception_directory_base<>;
using exception_directory_details = exception_directory_base<error_list>;

} //namespace pe_bliss::exceptions
