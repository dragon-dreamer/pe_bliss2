#include "pe_bliss2/error_list.h"

#include <algorithm>

namespace pe_bliss
{

void error_list::add_error(const std::error_code& error)
{
	if (std::find(errors_.cbegin(), errors_.cend(), error)
		== errors_.cend())
	{
		errors_.emplace_back(error);
	}
}

} //namespace pe_bliss
