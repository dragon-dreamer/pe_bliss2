#include "pe_bliss2/section/section_search.h"

#include <cstdint>
#include <limits>

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "pe_bliss2/section/section_header.h"
#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace pe_bliss::section
{

by_raw_offset::by_raw_offset(std::uint32_t raw_offset,
	std::uint32_t section_alignment,
	std::uint32_t data_size)
	: raw_offset_(raw_offset)
	, section_alignment_(section_alignment)
	, data_size_(data_size)
{
	if (!utilities::math::is_sum_safe(raw_offset_, data_size_))
		throw pe_error(utilities::generic_errc::integer_overflow);
}

bool by_raw_offset::operator()(const section_header& header) const noexcept
{
	auto start_offset = header.get_pointer_to_raw_data();
	auto end_offset = start_offset;
	if (!utilities::math::add_if_safe(end_offset,
		header.get_raw_size(section_alignment_)))
	{
		end_offset = (std::numeric_limits<std::uint32_t>::max)();
	}
	return raw_offset_ >= start_offset && (raw_offset_ + data_size_ <= end_offset);
}

by_rva::by_rva(rva_type rva, std::uint32_t section_alignment,
	std::uint32_t data_size)
	: rva_(rva)
	, section_alignment_(section_alignment)
	, data_size_(data_size)
{
	if (!utilities::math::is_sum_safe(rva_, data_size_))
		throw pe_error(utilities::generic_errc::integer_overflow);
}

bool by_rva::operator()(const section_header& header) const noexcept
{
	auto start_rva = header.get_rva();
	auto end_rva = start_rva;
	if (!utilities::math::add_if_safe(end_rva,
		header.get_virtual_size(section_alignment_)))
	{
		end_rva = (std::numeric_limits<rva_type>::max)();
	}
	return rva_ >= start_rva && (rva_ + data_size_ <= end_rva);
}

by_pointer::by_pointer(const section_header* ptr) noexcept
	: ptr_(ptr)
{
}

bool by_pointer::operator()(const section_header& header) const noexcept
{
	return &header == ptr_;
}

} //namespace pe_bliss::section
