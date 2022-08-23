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
		case invalid_version:
			return "Invalid version";
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

template<bool HasCondition>
std::uint32_t epilog_info<HasCondition>::get_epilog_start_offset() const noexcept
{
	return (descriptor_.get() & 0x3ffffu) * 2u;
}

template<bool HasCondition>
typename epilog_info<HasCondition>::epilog_start_index_type
epilog_info<HasCondition>::get_epilog_start_index() const noexcept
{
	return static_cast<epilog_start_index_type>(
		(descriptor_.get() & epilog_start_index_mask)
		>> epilog_start_index_shift);
}

template<bool HasCondition>
std::uint8_t epilog_info<HasCondition>::get_epilog_condition() const noexcept
	requires (HasCondition)
{
	return static_cast<std::uint8_t>((descriptor_.get() & 0xf00000u) >> 20u);
}

template class epilog_info<false>;
template class epilog_info<true>;

std::uint8_t extended_unwind_record_base::get_version() const noexcept
{
	return static_cast<std::uint8_t>((main_header_.get() & 0xc0000u) >> 18u);
}

bool extended_unwind_record_base::has_exception_data() const noexcept
{
	return (main_header_.get() & 0x100000u) != 0u;
}

bool extended_unwind_record_base::single_epilog_info_packed() const noexcept
{
	return (main_header_.get() & 0x200000u) != 0u;
}

void extended_unwind_record_base::set_function_length(std::uint32_t length)
{
	if (length % 4u)
		throw pe_error(exception_directory_errc::invalid_function_length);

	length /= 4u;
	if (length > 0x3ffffu)
		throw pe_error(exception_directory_errc::invalid_function_length);

	main_header_.get() &= ~0x3ffffu;
	main_header_.get() |= length;
}

void extended_unwind_record_base::set_version(std::uint8_t version)
{
	if (version != 0u)
		throw pe_error(exception_directory_errc::invalid_version);

	main_header_.get() &= ~0xc0000u;
}

void extended_unwind_record_base::set_has_exception_data(bool value) noexcept
{
	if (value)
		main_header_.get() |= 0x100000u;
	else
		main_header_.get() &= ~0x100000u;
}

void extended_unwind_record_base::set_single_epilog_info_packed(bool value) noexcept
{
	if (value)
		main_header_.get() |= 0x200000u;
	else
		main_header_.get() &= ~0x200000u;
}

std::uint16_t extended_unwind_record_base::get_extended_epilog_count() const noexcept
{
	return static_cast<std::uint16_t>(
		main_extended_header_.get() & 0xffffu);
}

std::uint8_t extended_unwind_record_base::get_extended_code_words() const noexcept
{
	return static_cast<std::uint8_t>(
		(main_extended_header_.get() & 0xff0000u) >> 16u);
}

} //namespace namespace pe_bliss::exceptions::arm_common
