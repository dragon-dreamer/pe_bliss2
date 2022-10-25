#pragma once

namespace pe_bliss::exceptions::x64
{
template<std::size_t Nodes>
opcode_id opcode_base<Nodes>::get_uwop_code() const noexcept
{
	return static_cast<opcode_id>(
		this->descriptor_->unwind_operation_code_and_info & 0xfu);
}

template<std::size_t Nodes>
std::uint8_t opcode_base<Nodes>::get_operation_info() const noexcept
{
	return (this->descriptor_->unwind_operation_code_and_info & 0xf0u) >> 4u;
}

template<std::size_t Nodes>
void opcode_base<Nodes>::set_uwop_code(opcode_id opcode) noexcept
{
	this->descriptor_->unwind_operation_code_and_info &= ~0xfu;
	this->descriptor_->unwind_operation_code_and_info
		|= static_cast<std::uint8_t>(opcode) & 0xfu;
}

template<std::size_t Nodes>
void opcode_base<Nodes>::set_operation_info(std::uint8_t info)
{
	if (info > 0xfu)
		throw pe_error(exception_directory_errc::invalid_operation_info);

	this->descriptor_->unwind_operation_code_and_info &= ~0xf0u;
	this->descriptor_->unwind_operation_code_and_info |= info << 4u;
}

template<std::size_t Nodes>
register_id opcode_base_with_register<Nodes>::get_register() const noexcept
{
	return static_cast<register_id>(this->get_operation_info());
}

template<std::size_t Nodes>
void opcode_base_with_register<Nodes>::set_register(register_id reg)
{
	this->set_operation_info(static_cast<std::uint8_t>(reg));
}

inline std::uint32_t alloc_large<1u>::get_allocation_size() const noexcept
{
	return static_cast<std::uint32_t>(get_descriptor()->node) * 8u;
}

inline std::uint32_t alloc_large<2u>::get_allocation_size() const noexcept
{
	return get_descriptor()->node;
}

inline void alloc_large<2u>::set_allocation_size(std::uint32_t size) noexcept
{
	get_descriptor()->node = size;
}

inline std::uint8_t alloc_small::get_allocation_size() const noexcept
{
	return get_operation_info() * 8u + 8u;
}

inline std::uint32_t save_nonvol::get_stack_offset() const noexcept
{
	return static_cast<std::uint32_t>(get_descriptor()->node) * 8u;
}

inline std::uint32_t save_nonvol_far::get_stack_offset() const noexcept
{
	return get_descriptor()->node;
}

inline void save_nonvol_far::set_stack_offset(std::uint32_t offset) noexcept
{
	get_descriptor()->node = offset;
}

inline std::uint8_t epilog::get_size() const noexcept
{
	return get_operation_info();
}

inline void epilog::set_size(std::uint8_t size)
{
	set_operation_info(size);
}

inline std::uint32_t save_xmm128::get_stack_offset() const noexcept
{
	return static_cast<std::uint32_t>(get_descriptor()->node) * 16u;
}

inline std::uint32_t save_xmm128_far::get_stack_offset() const noexcept
{
	return get_descriptor()->node;
}

inline void save_xmm128_far::set_stack_offset(std::uint32_t offset) noexcept
{
	get_descriptor()->node = offset;
}

inline bool push_machframe::push_error_code() const noexcept
{
	return !!get_operation_info();
}

inline std::uint8_t push_machframe::get_rsp_decrement() const noexcept
{
	return push_error_code() ? 48u : 40u;
}

inline void push_machframe::set_push_error_code(bool value) noexcept
{
	set_operation_info(value ? 1u : 0u);
}

inline std::uint64_t set_fpreg_large::get_offset() const noexcept
{
	return get_descriptor()->node * 16ull;
}

inline unwind_info::unwind_code_list_type& unwind_info
	::get_unwind_code_list() & noexcept
{
	return unwind_code_list_;
}

inline const unwind_info::unwind_code_list_type& unwind_info
	::get_unwind_code_list() const& noexcept
{
	return unwind_code_list_;
}

inline unwind_info::unwind_code_list_type unwind_info
	::get_unwind_code_list() && noexcept
{
	return std::move(unwind_code_list_);
}

inline unwind_flags::value unwind_info::get_unwind_flags() const noexcept
{
	return static_cast<unwind_flags::value>(
		(descriptor_->flags_and_version & 0xf8u) >> 3u);
}

inline std::uint8_t unwind_info::get_version() const noexcept
{
	return descriptor_->flags_and_version & 0x7u;
}

inline std::uint8_t unwind_info::get_scaled_frame_register_offset() const noexcept
{
	return descriptor_->frame_register_and_offset & 0xf0u;
}

inline void unwind_info::clear_frame_register() noexcept
{
	descriptor_->frame_register_and_offset &= ~0xfu;
}

template<typename... Bases>
unwind_info& runtime_function_base<Bases...>::get_unwind_info() noexcept
{
	return unwind_info_;
}

template<typename... Bases>
const unwind_info& runtime_function_base<Bases...>::get_unwind_info() const noexcept
{
	return unwind_info_;
}

template<typename... Bases>
typename runtime_function_base<Bases...>::additional_info_type&
	runtime_function_base<Bases...>::get_additional_info() & noexcept
{
	return additional_info_;
}

template<typename... Bases>
const typename runtime_function_base<Bases...>::additional_info_type&
	runtime_function_base<Bases...>::get_additional_info() const& noexcept
{
	return additional_info_;
}

template<typename... Bases>
typename runtime_function_base<Bases...>::additional_info_type
	runtime_function_base<Bases...>::get_additional_info() && noexcept
{
	return std::move(additional_info_);
}

template<typename... Bases>
typename exception_directory_base<Bases...>::runtime_function_list_type&
	exception_directory_base<Bases...>::get_runtime_function_list() & noexcept
{
	return runtime_function_list_;
}

template<typename... Bases>
const typename exception_directory_base<Bases...>::runtime_function_list_type&
	exception_directory_base<Bases...>::get_runtime_function_list() const& noexcept
{
	return runtime_function_list_;
}

template<typename... Bases>
typename exception_directory_base<Bases...>::runtime_function_list_type
	exception_directory_base<Bases...>::get_runtime_function_list() && noexcept
{
	return std::move(runtime_function_list_);
}
} //namespace pe_bliss::exceptions::x64
