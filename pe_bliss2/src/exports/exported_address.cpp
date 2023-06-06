#include "pe_bliss2/exports/exported_address.h"

namespace pe_bliss::exports
{

forwarded_name_info get_forwarded_name_info(const std::string& forwarded_name)
{
	auto pos = forwarded_name.find('.');
	if (pos == std::string::npos)
		return { .function_name = forwarded_name };

	return {
		.library_name = forwarded_name.substr(0, pos),
		.function_name = forwarded_name.substr(pos + 1)
	};
}

} //namespace pe_bliss::exports
