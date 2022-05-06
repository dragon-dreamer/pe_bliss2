#include "pe_bliss2/section/pe_section_error.h"

#include <cstddef>
#include <string>
#include <system_error>
#include <utility>

namespace pe_bliss::section
{

pe_section_error::pe_section_error(std::error_code ec,
	std::size_t section_index, std::string section_name)
	: pe_error(ec)
	, section_index_(section_index)
	, section_name_(std::move(section_name))
{
}

} //namespace pe_bliss::section
