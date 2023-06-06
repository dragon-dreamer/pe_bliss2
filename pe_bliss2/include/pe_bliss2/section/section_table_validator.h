#pragma once

#include <cstddef>

#include "pe_bliss2/section/section_table.h"

namespace pe_bliss
{
class error_list;

namespace core
{
class optional_header;
} //namespace core
} //namespace pe_bliss

namespace pe_bliss::section
{

class section_header;

// Validates a particular header without taking
// other headers into consideration. Thus, does not validate
// raw size alignment and potential virtual gaps.
void validate_section_header(const core::optional_header& oh,
	const section_header& header, std::size_t section_index,
	error_list& errors);

void validate_section_header(const core::optional_header& oh,
	section_table::header_list::const_iterator header_it,
	const section_table::header_list& headers,
	error_list& errors);

void validate_section_headers(const core::optional_header& oh,
	const section_table::header_list& headers,
	error_list& errors);

} //namespace pe_bliss::section
