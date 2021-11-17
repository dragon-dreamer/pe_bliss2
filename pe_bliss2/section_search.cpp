#include "pe_bliss2/section_search.h"

#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace pe_bliss::section_search
{

by_raw_offset::by_raw_offset(std::uint32_t raw_offset, std::uint32_t section_alignment,
	std::uint32_t data_size)
	: raw_offset_(raw_offset)
	, section_alignment_(section_alignment)
	, data_size_(data_size)
{
	if (!utilities::math::is_sum_safe(raw_offset_, data_size_))
		throw pe_error(utilities::generic_errc::integer_overflow);
}

bool by_raw_offset::operator()(const section_header& header) const
{
	auto start_offset = header.get_pointer_to_raw_data();
	utilities::safe_uint end_offset(start_offset);
	end_offset += header.get_raw_size(section_alignment_);
	return raw_offset_ >= start_offset && (raw_offset_ + data_size_ <= end_offset.value());
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

bool by_rva::operator()(const section_header& header) const
{
	auto start_rva = header.get_rva();
	utilities::safe_uint end_rva(start_rva);
	end_rva += header.get_virtual_size(section_alignment_);
	return rva_ >= start_rva && (rva_ + data_size_ <= end_rva.value());
}

by_pointer::by_pointer(const section_header* ptr)
	: ptr_(ptr)
{
}

bool by_pointer::operator()(const section_header& header) const noexcept
{
	return &header == ptr_;
}

} //namespace pe_bliss::section_search
