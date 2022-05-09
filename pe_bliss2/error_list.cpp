#include "pe_bliss2/error_list.h"

#include <algorithm>

namespace pe_bliss
{

void error_list::add_error(std::error_code error)
{
	if (!has_error(error))
		errors_.emplace_back(error);
}

bool error_list::has_error(std::error_code error) const noexcept
{
	return std::find(errors_.cbegin(), errors_.cend(), error)
		!= errors_.cend();
}

} //namespace pe_bliss
