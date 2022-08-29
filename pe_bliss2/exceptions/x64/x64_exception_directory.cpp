#include "pe_bliss2/exceptions/x64/x64_exception_directory.h"

#include <system_error>

namespace
{

struct x64_exception_directory_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "x64_exception_directory";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::exceptions::x64::exception_directory_errc;
		switch (static_cast<pe_bliss::exceptions::x64::exception_directory_errc>(ev))
		{
		case invalid_operation_info:
			return "Invalid operation info";
		case invalid_allocation_size:
			return "Invalid allocation size";
		case invalid_stack_offset:
			return "Invalid stack offset";
		case invalid_unwind_flags:
			return "Invalid unwind flags combination";
		case invalid_unwind_info_version:
			return "Invalid unwind info version";
		case invalid_frame_register:
			return "Invalid frame register";
		case invalid_scaled_frame_register_offset:
			return "Invalid scaled frame register offset";
		default:
			return {};
		}
	}
};

const x64_exception_directory_error_category x64_exception_directory_error_category_instance;

} //namespace

namespace pe_bliss::exceptions::x64
{

std::error_code make_error_code(exception_directory_errc e) noexcept
{
	return { static_cast<int>(e), x64_exception_directory_error_category_instance };
}

void alloc_large<1u>::set_allocation_size(std::uint32_t size)
{
	if (size % 8u)
		throw pe_error(exception_directory_errc::invalid_allocation_size);

	get_descriptor()->node = static_cast<std::uint16_t>(size / 8u);
}

void alloc_small::set_allocation_size(std::uint8_t size)
{
	if (size < 8u || (size - 8u) % 8u)
		throw pe_error(exception_directory_errc::invalid_allocation_size);

	set_operation_info((size - 8u) / 8u);
}

void save_nonvol::set_stack_offset(std::uint32_t offset)
{
	if (offset % 8u)
		throw pe_error(exception_directory_errc::invalid_stack_offset);

	get_descriptor()->node = static_cast<std::uint16_t>(offset / 8u);
}

void save_xmm128::set_stack_offset(std::uint32_t offset)
{
	if (offset % 16u)
		throw pe_error(exception_directory_errc::invalid_stack_offset);

	get_descriptor()->node = static_cast<std::uint16_t>(offset / 16u);
}

void set_fpreg_large::set_offset(std::uint32_t offset)
{
	if (offset & 0xfu)
		throw pe_error(exception_directory_errc::invalid_scaled_frame_register_offset);

	get_descriptor()->node = offset / 16u;
}

std::optional<register_id> unwind_info::get_frame_register() const noexcept
{
	auto fr = descriptor_->frame_register_and_offset & 0xfu;
	std::optional<register_id> result;
	if (fr)
		result = static_cast<register_id>(fr);

	return result;
}

void unwind_info::set_unwind_flags(unwind_flags::value flags)
{
	if ((flags & unwind_flags::chaininfo)
		&& (flags & (unwind_flags::ehandler | unwind_flags::uhandler)))
	{
		throw pe_error(exception_directory_errc::invalid_unwind_flags);
	}

	descriptor_->flags_and_version &= ~0xf8u;
	descriptor_->flags_and_version |= (flags << 3u);
}

void unwind_info::set_version(std::uint8_t version)
{
	if (version > 0x7u)
		throw pe_error(exception_directory_errc::invalid_unwind_info_version);

	descriptor_->flags_and_version &= ~0x7u;
	descriptor_->flags_and_version |= (version & 0x7u);
}

void unwind_info::set_frame_register(register_id fr)
{
	if (static_cast<std::uint8_t>(fr) > 0xfu || fr == register_id::rax)
		throw pe_error(exception_directory_errc::invalid_frame_register);

	descriptor_->frame_register_and_offset &= ~0xfu;
	descriptor_->frame_register_and_offset |= static_cast<std::uint8_t>(fr);
}

void unwind_info::set_scaled_frame_register_offset(std::uint8_t offset)
{
	if (offset & 0xfu)
		throw pe_error(exception_directory_errc::invalid_scaled_frame_register_offset);

	descriptor_->frame_register_and_offset &= ~0xf0u;
	descriptor_->frame_register_and_offset |= offset;
}

} //namespace pe_bliss::exceptions::x64
