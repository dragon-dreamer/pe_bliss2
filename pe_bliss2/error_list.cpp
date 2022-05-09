#include "pe_bliss2/error_list.h"

#include <algorithm>
#include <utility>

namespace pe_bliss
{

void error_list::add_error(std::error_code error)
{
	if (!has_error(error))
		errors_.emplace_back(error_info{ error, {} });
}

void error_list::add_error(std::error_code error, std::string context)
{
	if (!has_error(error, context))
		errors_.emplace_back(error_info{ error, std::move(context) });
}

bool error_list::has_error(std::error_code error) const noexcept
{
	return has_error(error, {});
}

bool error_list::has_error(std::error_code error,
	std::string_view context) const noexcept
{
	return std::find_if(errors_.cbegin(), errors_.cend(),
	[error, context](const auto& info) {
		return info.code == error && info.context == context;
	}) != errors_.cend();
}

bool error_list::has_any_error(std::error_code error) const noexcept
{
	return std::find_if(errors_.cbegin(), errors_.cend(),
	[error](const auto& info) {
		return info.code == error;
	}) != errors_.cend();
}

} //namespace pe_bliss
