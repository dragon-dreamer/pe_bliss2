#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/detail/exceptions/image_runtime_function_entry.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"

#include "utilities/static_class.h"

namespace pe_bliss::exceptions::x64
{

enum class exception_directory_errc
{
	invalid_operation_info = 1,
	invalid_allocation_size,
	invalid_stack_offset,
	invalid_unwind_flags,
	invalid_unwind_info_version,
	invalid_frame_register,
	invalid_scaled_frame_register_offset
};

} //namespace pe_bliss::exceptions::x64

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::exceptions::x64::exception_directory_errc> : true_type {};
} //namespace std

namespace pe_bliss::exceptions::x64
{

std::error_code make_error_code(exception_directory_errc) noexcept;

struct unwind_flags final : utilities::static_class
{
	enum value : std::uint8_t
	{
		ehandler = detail::exceptions::unw_flag_ehandler,
		uhandler = detail::exceptions::unw_flag_uhandler,
		chaininfo = detail::exceptions::unw_flag_chaininfo
	};
};

enum class register_id : std::uint8_t
{
	rax,
	rcx,
	rdx,
	rbx,
	rsp,
	rbp,
	rsi,
	rdi,
	r8,
	r9,
	r10,
	r11,
	r12,
	r13,
	r14,
	r15
};

enum class opcode_id : std::uint8_t
{
	push_nonvol = 0,
	alloc_large = 1,
	alloc_small = 2,
	set_fpreg = 3,
	save_nonvol = 4,
	save_nonvol_far = 5,
	epilog = 6,
	spare = 7,
	save_xmm128 = 8,
	save_xmm128_far = 9,
	push_machframe = 10,
	set_fpreg_large = 11
};

template<std::size_t Nodes = 0u>
class [[nodiscard]] opcode_base
	: public detail::packed_struct_base<detail::exceptions::unwind_code<Nodes>>
{
public:
	static constexpr std::uint32_t node_count = Nodes;

public:
	static_assert(Nodes <= 2u);

public:
	[[nodiscard]]
	opcode_id get_uwop_code() const noexcept;

	[[nodiscard]]
	std::uint8_t get_operation_info() const noexcept;

public:
	void set_uwop_code(opcode_id opcode) noexcept;
	void set_operation_info(std::uint8_t info);
};

template<opcode_id Opcode>
class [[nodiscard]] opcode_id_base
{
public:
	static constexpr auto opcode = Opcode;
};

template<std::size_t Nodes = 0u>
class [[nodiscard]] opcode_base_with_register : public opcode_base<Nodes>
{
public:
	[[nodiscard]]
	register_id get_register() const noexcept;

public:
	void set_register(register_id reg);
};

//Push a nonvolatile integer register, decrementing RSP by 8.
//The operation info is the number of the register.
//Because of the constraints on epilogs, UWOP_PUSH_NONVOL unwind codes must appear
//first in the prolog and correspondingly, last in the unwind code array.
//This relative ordering applies to all other unwind codes except UWOP_PUSH_MACHFRAME.
class [[nodiscard]] push_nonvol
	: public opcode_base_with_register<>
	, public opcode_id_base<opcode_id::push_nonvol>
{
};

//Allocate a large-sized area on the stack.
//There are two forms. If the operation info equals 0, then the size of the allocation
//divided by 8 is recorded in the next slot, allowing an allocation up to 512K - 8.
//If the operation info equals 1, then the unscaled size of the allocation is recorded
//in the next two slots in little-endian format, allowing allocations up to 4GB - 8.
template<std::size_t Nodes>
class [[nodiscard]] alloc_large
{
	static_assert(Nodes == 1u || Nodes == 2u);
};

template<>
class [[nodiscard]] alloc_large<1u>
	: public opcode_base<1u>
	, public opcode_id_base<opcode_id::alloc_large>
{
public:
	[[nodiscard]]
	inline std::uint32_t get_allocation_size() const noexcept;
	
	void set_allocation_size(std::uint32_t size);
};

template<>
class [[nodiscard]] alloc_large<2u>
	: public opcode_base<2u>
	, public opcode_id_base<opcode_id::alloc_large>
{
public:
	[[nodiscard]]
	inline std::uint32_t get_allocation_size() const noexcept;

	inline void set_allocation_size(std::uint32_t size) noexcept;
};

//Allocate a small-sized area on the stack.
//The size of the allocation is the operation info field * 8 + 8,
//allowing allocations from 8 to 128 bytes.
class [[nodiscard]] alloc_small
	: public opcode_base<>
	, public opcode_id_base<opcode_id::alloc_small>
{
public:
	[[nodiscard]]
	inline std::uint8_t get_allocation_size() const noexcept;

	void set_allocation_size(std::uint8_t size);
};

//Establish the frame pointer register by setting the register to some offset of the current RSP.
//The offset is equal to the Frame Register offset (scaled) field in the UNWIND_INFO * 16,
//allowing offsets from 0 to 240.
//The use of an offset permits establishing a frame pointer that points
//to the middle of the fixed stack allocation,
//helping code density by allowing more accesses to use short instruction forms.
//The operation info field is reserved and shouldn't be used.
class [[nodiscard]] set_fpreg
	: public opcode_base<>
	, public opcode_id_base<opcode_id::set_fpreg>
{
};

//Save a nonvolatile integer register on the stack using a MOV instead of a PUSH.
//This code is primarily used for shrink-wrapping,
//where a nonvolatile register is saved to the stack in a position
//that was previously allocated.
//The operation info is the number of the register.
//The scaled-by-8 stack offset is recorded in the next unwind operation code slot,
//as described in the note above.
class [[nodiscard]] save_nonvol
	: public opcode_base_with_register<1u>
	, public opcode_id_base<opcode_id::save_nonvol>
{
public:
	[[nodiscard]]
	inline std::uint32_t get_stack_offset() const noexcept;

	void set_stack_offset(std::uint32_t offset);
};

//Save a nonvolatile integer register on the stack with a long offset,
//using a MOV instead of a PUSH. This code is primarily used for shrink-wrapping,
//where a nonvolatile register is saved to the stack in a position
//that was previously allocated.
//The operation info is the number of the register.
//The unscaled stack offset is recorded in the next two unwind operation code slots,
//as described in the note above.
class [[nodiscard]] save_nonvol_far
	: public opcode_base_with_register<2u>
	, public opcode_id_base<opcode_id::save_nonvol_far>
{
public:
	[[nodiscard]]
	inline std::uint32_t get_stack_offset() const noexcept;

	inline void set_stack_offset(std::uint32_t offset) noexcept;
};

//UWOP_SAVE_XMM for UNWIND_INFO v1 (2 slots), retains the lower 64 bits of
//the XMM register. Currently ignored.
//UWOP_EPILOG for UNWIND_INFO v2 (2 slots), describes the function epilogue.
class [[nodiscard]] epilog
	: public opcode_base<>
	, public opcode_id_base<opcode_id::epilog>
{
public:
	[[nodiscard]]
	inline std::uint8_t get_size() const noexcept;

	inline void set_size(std::uint8_t size);
};

//UWOP_SAVE_XMM_FAR for UNWIND_INFO v1, saves the lower 64 bits of
//the XMM register. Currently ignored.
//UWOP_SPARE_CODE for UNWIND_INFO v2, ignored.
class [[nodiscard]] spare
	: public opcode_base<2u>
	, public opcode_id_base<opcode_id::spare>
{
};

//Save all 128 bits of a nonvolatile XMM register on the stack.
//The operation info is the number of the register.
//The scaled-by-16 stack offset is recorded in the next slot.
class [[nodiscard]] save_xmm128
	: public opcode_base_with_register<1u>
	, public opcode_id_base<opcode_id::save_nonvol_far>
{
public:
	[[nodiscard]]
	inline std::uint32_t get_stack_offset() const noexcept;

	void set_stack_offset(std::uint32_t offset);
};

//Save all 128 bits of a nonvolatile XMM register on the stack with a long offset.
//The operation info is the number of the register.
//The unscaled stack offset is recorded in the next two slots.
class [[nodiscard]] save_xmm128_far
	: public opcode_base_with_register<2u>
	, public opcode_id_base<opcode_id::save_xmm128_far>
{
public:
	[[nodiscard]]
	std::uint32_t get_stack_offset() const noexcept;

	void set_stack_offset(std::uint32_t offset) noexcept;
};

//Push a machine frame. This unwind code is used to record the effect
//of a hardware interrupt or exception.
//There are two forms. If the operation info equals 0,
//one of these frames has been pushed on the stack:
//SS, Old RSP, EFLAGS, CS, RIP.
//If the operation info equals 1, then one of these frames has been pushed:
//SS, Old RSP, EFLAGS, CS, RIP, Error code.
//This unwind code always appears in a dummy prolog, which is never actually executed,
//but instead appears before the real entry point of an interrupt routine,
//and exists only to provide a place to simulate the push of a machine frame.
//The simulated UWOP_PUSH_MACHFRAME operation decrements
//RSP by 40 (op info equals 0) or 48 (op info equals 1).
class [[nodiscard]] push_machframe
	: public opcode_base<>
	, public opcode_id_base<opcode_id::push_machframe>
{
public:
	[[nodiscard]]
	inline bool push_error_code() const noexcept;

	[[nodiscard]]
	inline std::uint8_t get_rsp_decrement() const noexcept;

	inline void set_push_error_code(bool value) noexcept;
};

//CLR Unix-only extension. When used, frame_register must be set to
//the frame pointer register, and frame_offset must be set to
//15. Followed by two unwind codes that are combined to form a 32-bit offset (as
//save_nonvol_far). This offset is then scaled by 16. The result must
//be less than 2^32 (top 4 bits of the unscaled 32-bit number
//must be zero). The scaled value is used as frame pointer register offset
//from RSP at the time the frame pointer is established. Either
//set_fpreg or set_fpreg_large may be used, but not both.
class [[nodiscard]] set_fpreg_large
	: public opcode_base<2u>
	, public opcode_id_base<opcode_id::set_fpreg_large>
{
public:
	[[nodiscard]]
	inline std::uint64_t get_offset() const noexcept;

	void set_offset(std::uint64_t offset);
};

class [[nodiscard]] unwind_info
	: public detail::packed_struct_base<detail::exceptions::unwind_info>
{
public:
	using unwind_code_type = std::variant<push_nonvol, alloc_large<1u>, alloc_large<2u>,
		alloc_small, set_fpreg, save_nonvol, save_nonvol_far,
		epilog, spare, save_xmm128, save_xmm128_far,
		push_machframe, set_fpreg_large>;
	using unwind_code_list_type = std::vector<unwind_code_type>;

public:
	[[nodiscard]]
	inline unwind_code_list_type& get_unwind_code_list() & noexcept;
	[[nodiscard]]
	inline const unwind_code_list_type& get_unwind_code_list() const& noexcept;
	[[nodiscard]]
	inline unwind_code_list_type get_unwind_code_list() && noexcept;

public:
	[[nodiscard]]
	inline unwind_flags::value get_unwind_flags() const noexcept;

	[[nodiscard]]
	inline std::uint8_t get_version() const noexcept;

	[[nodiscard]]
	std::optional<register_id> get_frame_register() const noexcept;

	[[nodiscard]]
	inline std::uint8_t get_scaled_frame_register_offset() const noexcept;

	void set_unwind_flags(unwind_flags::value flags);

	void set_version(std::uint8_t version);

	void set_frame_register(register_id fr);

	inline void clear_frame_register() noexcept;

	void set_scaled_frame_register_offset(std::uint8_t offset);

private:
	unwind_code_list_type unwind_code_list_;
};

template<typename... Bases>
class [[nodiscard]] runtime_function_base
	: public detail::packed_struct_base<detail::exceptions::image_runtime_function_entry>
	, public Bases...
{
public:
	using runtime_function_ptr = std::unique_ptr<runtime_function_base<Bases...>>;
	using exception_handler_rva_type = packed_struct<rva_type>;
	using additional_info_type = std::variant<std::monostate,
		exception_handler_rva_type, runtime_function_ptr>;

public:
	[[nodiscard]]
	unwind_info& get_unwind_info() noexcept;
	[[nodiscard]]
	const unwind_info& get_unwind_info() const noexcept;

	[[nodiscard]]
	additional_info_type& get_additional_info() & noexcept;
	[[nodiscard]]
	const additional_info_type& get_additional_info() const& noexcept;
	[[nodiscard]]
	additional_info_type get_additional_info() && noexcept;

private:
	unwind_info unwind_info_;
	additional_info_type additional_info_;
};

template<typename... Bases>
class [[nodiscard]] exception_directory_base
	: public Bases...
{
public:
	using runtime_function_list_type = std::vector<runtime_function_base<Bases...>>;

public:
	[[nodiscard]]
	runtime_function_list_type& get_runtime_function_list() & noexcept;
	[[nodiscard]]
	const runtime_function_list_type& get_runtime_function_list() const& noexcept;
	[[nodiscard]]
	runtime_function_list_type get_runtime_function_list() && noexcept;

private:
	runtime_function_list_type runtime_function_list_;
};

using runtime_function = runtime_function_base<>;
using runtime_function_details = runtime_function_base<error_list>;

using exception_directory = exception_directory_base<>;
using exception_directory_details = exception_directory_base<error_list>;

} //namespace pe_bliss::exceptions::x64

#include "pe_bliss2/exceptions/x64/x64_exception_directory-inl.h"
