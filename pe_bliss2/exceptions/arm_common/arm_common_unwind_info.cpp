#include "pe_bliss2/exceptions/arm_common/arm_common_unwind_info.h"

#include <string>
#include <system_error>

namespace
{

struct arm_common_exception_directory_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "arm_common_exception_directory";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::exceptions::arm_common::exception_directory_errc;
		switch (static_cast<pe_bliss::exceptions::arm_common::exception_directory_errc>(ev))
		{
		case invalid_epilog_start_offset:
			return "Invalid epilog start offset";
		case invalid_epilog_start_index:
			return "Invalid epilog start index";
		case invalid_epilog_condition:
			return "Invalid epilog condition";
		case invalid_function_length:
			return "Invalid function length";
		default:
			return {};
		}
	}
};

const arm_common_exception_directory_error_category arm_common_exception_directory_error_category_instance;

} //namespace

namespace pe_bliss::exceptions::arm_common
{

std::error_code make_error_code(exception_directory_errc e) noexcept
{
	return { static_cast<int>(e), arm_common_exception_directory_error_category_instance };
}

template<bool HasCondition>
void epilog_info<HasCondition>::set_epilog_start_offset(std::uint32_t offset)
{
	if (offset % 2u)
		throw pe_error(exception_directory_errc::invalid_epilog_start_offset);

	offset /= 2u;
	if (offset > 0x3ffffu)
		throw pe_error(exception_directory_errc::invalid_epilog_start_offset);

	descriptor_.get() &= ~0x3ffffu;
	descriptor_.get() |= offset;
}

template<bool HasCondition>
void epilog_info<HasCondition>::set_epilog_start_index(epilog_start_index_type index)
{
	if (index > (epilog_start_index_mask >> epilog_start_index_shift))
		throw pe_error(exception_directory_errc::invalid_epilog_start_index);

	descriptor_.get() &= ~epilog_start_index_mask;
	descriptor_.get() |= index << epilog_start_index_shift;
}

template<bool HasCondition>
void epilog_info<HasCondition>::set_epilog_condition(std::uint8_t condition)
	requires (HasCondition)
{
	if (condition > 0xfu)
		throw pe_error(exception_directory_errc::invalid_epilog_condition);

	descriptor_.get() &= ~0xf00000u;
	descriptor_.get() |= static_cast<std::uint32_t>(condition) << 20u;
}

template class epilog_info<false>;
template class epilog_info<true>;

} //namespace namespace pe_bliss::exceptions::arm_common
