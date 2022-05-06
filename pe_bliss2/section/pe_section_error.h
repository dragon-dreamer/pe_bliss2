#pragma once

#include <cstddef>
#include <string>
#include <system_error>

#include "pe_bliss2/pe_error.h"

namespace pe_bliss::section
{

class [[nodiscard]] pe_section_error : public pe_error
{
public:
	pe_section_error(std::error_code ec,
		std::size_t section_index, std::string section_name);

	[[nodiscard]] std::size_t get_section_index() const noexcept
	{
		return section_index_;
	}

	[[nodiscard]] const std::string& get_section_name() const noexcept
	{
		return section_name_;
	}

private:
	std::size_t section_index_;
	std::string section_name_;
};

} //namespace pe_bliss::section
