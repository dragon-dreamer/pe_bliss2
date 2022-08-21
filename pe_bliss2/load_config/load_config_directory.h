#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "pe_bliss2/bit_stream.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/detail/load_config/image_load_config_directory.h"
#include "pe_bliss2/packed_byte_array.h"
#include "pe_bliss2/packed_byte_vector.h"
#include "pe_bliss2/packed_c_string.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/detail/relocations/image_base_relocation.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"

#include "utilities/static_class.h"

namespace pe_bliss::load_config
{

enum class load_config_errc
{
	unknown_load_config_version = 1,
	invalid_stride_value,
	invalid_page_relative_offset,
	invalid_iat_index,
	invalid_size_value,
	invalid_meta_value,
	invalid_instr_size,
	invalid_disp_offset,
	invalid_disp_size
};

std::error_code make_error_code(load_config_errc) noexcept;

} //namespace pe_bliss::load_config

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::load_config::load_config_errc> : true_type {};
} //namespace std

namespace pe_bliss::load_config
{

enum class version
{
	base,
	seh,
	cf_guard,
	code_integrity,
	cf_guard_ex,
	hybrid_pe,
	rf_guard,
	rf_guard_ex,
	enclave,
	volatile_metadata,
	eh_guard,
	xf_guard,
	cast_guard_os_determined_failure_mode,
	guard_memcpy_function_pointer
};

[[nodiscard]]
const char* version_to_min_required_windows_version(version value) noexcept;

template<detail::executable_pointer Pointer>
class [[nodiscard]] lock_prefix_table
{
public:
	using pointer_type = Pointer;
	using lock_prefix_list_type = std::vector<packed_struct<pointer_type>>;

public:
	[[nodiscard]] lock_prefix_list_type& get_prefix_va_list() & noexcept;
	[[nodiscard]] const lock_prefix_list_type& get_prefix_va_list() const& noexcept;
	[[nodiscard]] lock_prefix_list_type get_prefix_va_list() && noexcept;

private:
	lock_prefix_list_type prefixes_;
};

class [[nodiscard]] handler_table
{
public:
	using handler_list_type = std::vector<packed_struct<rva_type>>;

public:
	[[nodiscard]] inline handler_list_type& get_handler_list() & noexcept;
	[[nodiscard]] inline const handler_list_type& get_handler_list() const& noexcept;
	[[nodiscard]] inline handler_list_type get_handler_list() && noexcept;

private:
	handler_list_type handlers_;
};

struct gfids_flags final : utilities::static_class
{
	enum value : std::uint8_t
	{
		fid_suppressed = detail::load_config::gfids_flags::fid_suppressed,
		export_suppressed = detail::load_config::gfids_flags::export_suppressed,
		fid_langexcpthandler = detail::load_config::gfids_flags::fid_langexcpthandler,
		fid_xfg = detail::load_config::gfids_flags::fid_xfg
	};
};

class [[nodiscard]] guard_function_common
{
public:
	static constexpr std::uint8_t max_guard_function_table_stride = 0xfu;

public:
	using additional_data_type = packed_byte_array<max_guard_function_table_stride>;
	using packed_rva_type = packed_struct<rva_type>;

public:
	[[nodiscard]]
	inline const packed_rva_type& get_rva() const noexcept;
	[[nodiscard]]
	inline packed_rva_type& get_rva() noexcept;

	[[nodiscard]]
	inline const additional_data_type& get_additional_data() const noexcept;
	[[nodiscard]]
	inline additional_data_type& get_additional_data() noexcept;
	
private:
	packed_rva_type rva_;
	additional_data_type flags_{};
};

template<typename... Bases>
class [[nodiscard]] guard_function_base
	: public guard_function_common
	, public Bases...
{
public:
	using type_based_hash_type = packed_struct<std::uint64_t>;
	using optional_type_based_hash_type = std::optional<type_based_hash_type>;

public:
	[[nodiscard]] gfids_flags::value get_flags() const noexcept;
	void set_flags(gfids_flags::value flags) noexcept;

	[[nodiscard]]
	const optional_type_based_hash_type& get_type_based_hash() const noexcept;
	[[nodiscard]]
	optional_type_based_hash_type& get_type_based_hash() noexcept;

private:
	optional_type_based_hash_type type_based_hash_;
};

struct global_flags final : utilities::static_class
{
	enum value : std::uint32_t
	{
		disable_dbgprint = detail::load_config::gflags::flg_disable_dbgprint,
		kernel_stack_trace_db = detail::load_config::gflags::flg_kernel_stack_trace_db,
		user_stack_trace_db = detail::load_config::gflags::flg_user_stack_trace_db,
		debug_initial_command = detail::load_config::gflags::flg_debug_initial_command,
		debug_initial_command_ex = detail::load_config::gflags::flg_debug_initial_command_ex,
		heap_disable_coalescing = detail::load_config::gflags::flg_heap_disable_coalescing,
		disable_page_kernel_stacks = detail::load_config::gflags::flg_disable_page_kernel_stacks,
		disable_protdlls = detail::load_config::gflags::flg_disable_protdlls,
		disable_stack_extension = detail::load_config::gflags::flg_disable_stack_extension,
		critsec_event_creation = detail::load_config::gflags::flg_critsec_event_creation,
		application_verifier = detail::load_config::gflags::flg_application_verifier,
		enable_handle_exceptions = detail::load_config::gflags::flg_enable_handle_exceptions,
		enable_close_exceptions = detail::load_config::gflags::flg_enable_close_exceptions,
		enable_csrdebug = detail::load_config::gflags::flg_enable_csrdebug,
		enable_exception_logging = detail::load_config::gflags::flg_enable_exception_logging,
		heap_enable_free_check = detail::load_config::gflags::flg_heap_enable_free_check,
		heap_validate_parameters = detail::load_config::gflags::flg_heap_validate_parameters,
		heap_enable_tagging = detail::load_config::gflags::flg_heap_enable_tagging,
		heap_enable_tag_by_dll = detail::load_config::gflags::flg_heap_enable_tag_by_dll,
		heap_enable_tail_check = detail::load_config::gflags::flg_heap_enable_tail_check,
		heap_validate_all = detail::load_config::gflags::flg_heap_validate_all,
		enable_kdebug_symbol_load = detail::load_config::gflags::flg_enable_kdebug_symbol_load,
		enable_handle_type_tagging = detail::load_config::gflags::flg_enable_handle_type_tagging,
		heap_page_allocs = detail::load_config::gflags::flg_heap_page_allocs,
		pool_enable_tagging = detail::load_config::gflags::flg_pool_enable_tagging,
		enable_system_crit_breaks = detail::load_config::gflags::flg_enable_system_crit_breaks,
		maintain_object_typelist = detail::load_config::gflags::flg_maintain_object_typelist,
		monitor_silent_process_exit = detail::load_config::gflags::flg_monitor_silent_process_exit,
		show_ldr_snaps = detail::load_config::gflags::flg_show_ldr_snaps,
		stop_on_exception = detail::load_config::gflags::flg_stop_on_exception,
		stop_on_hung_gui = detail::load_config::gflags::flg_stop_on_hung_gui,
		stop_on_unhandled_exception = detail::load_config::gflags::flg_stop_on_unhandled_exception
	};
};

struct process_heap_flags final : utilities::static_class
{
	enum value : std::uint32_t
	{
		heap_create_enable_execute = detail::load_config::heap_flags::heap_create_enable_execute,
		heap_generate_exceptions = detail::load_config::heap_flags::heap_generate_exceptions,
		heap_no_serialize = detail::load_config::heap_flags::heap_no_serialize
	};
};

struct dependent_load_flags final : utilities::static_class
{
	enum value : std::uint16_t
	{
		load_library_search_application_dir = detail::load_config
			::dependent_load_flags::load_library_search_application_dir,
		load_library_search_default_dirs = detail::load_config
			::dependent_load_flags::load_library_search_default_dirs,
		load_library_search_dll_load_dir = detail::load_config
			::dependent_load_flags::load_library_search_dll_load_dir,
		load_library_search_system32 = detail::load_config
			::dependent_load_flags::load_library_search_system32,
		load_library_search_user_dirs = detail::load_config
			::dependent_load_flags::load_library_search_user_dirs
	};
};

struct guard_flags final : utilities::static_class
{
	enum value : std::uint32_t
	{
		cf_instrumented = detail::load_config::guard_flags::cf_instrumented,
		cfw_instrumented = detail::load_config::guard_flags::cfw_instrumented,
		cf_function_table_present = detail::load_config::guard_flags::cf_function_table_present,
		security_cookie_unused = detail::load_config::guard_flags::security_cookie_unused,
		protect_delayload_iat = detail::load_config::guard_flags::protect_delayload_iat,
		delayload_iat_in_its_own_section = detail::load_config::guard_flags::delayload_iat_in_its_own_section,
		cf_export_suppression_info_present = detail::load_config::guard_flags::cf_export_suppression_info_present,
		cf_enable_export_suppression = detail::load_config::guard_flags::cf_enable_export_suppression,
		cf_longjump_table_present = detail::load_config::guard_flags::cf_longjump_table_present,
		rf_instrumented = detail::load_config::guard_flags::rf_instrumented,
		rf_enable = detail::load_config::guard_flags::rf_enable,
		rf_strict = detail::load_config::guard_flags::rf_strict,
		retpoline_present = detail::load_config::guard_flags::retpoline_present,
		eh_continuation_table_present_20h1 = detail::load_config::guard_flags::eh_continuation_table_present_20h1,
		eh_continuation_table_present = detail::load_config::guard_flags::eh_continuation_table_present,
		xfg_enabled = detail::load_config::guard_flags::xfg_enabled,
		castguard_present = detail::load_config::guard_flags::castguard_present,
		memcpy_present = detail::load_config::guard_flags::memcpy_present
	};
};

enum class dynamic_relocation_symbol
{
	guard_rf_prologue = detail::load_config::dynamic_relocation_symbol::guard_rf_prologue,
	guard_rf_epilogue = detail::load_config::dynamic_relocation_symbol::guard_rf_epilogue,
	guard_import_control_transfer = detail::load_config::dynamic_relocation_symbol::guard_import_control_transfer,
	guard_indir_control_transfer = detail::load_config::dynamic_relocation_symbol::guard_indir_control_transfer,
	guard_switchtable_branch = detail::load_config::dynamic_relocation_symbol::guard_switchtable_branch,
	guard_arm64x = detail::load_config::dynamic_relocation_symbol::guard_arm64x,
	function_override = detail::load_config::dynamic_relocation_symbol::function_override,
};

constexpr auto code_integrity_catalog_not_available = detail::load_config::code_integrity_catalog_not_available;

enum class chpe_arm64x_range_code_type : std::uint8_t
{
	arm64 = detail::load_config::chpe_arm64x_range_code_type_arm64,
	arm64ec = detail::load_config::chpe_arm64x_range_code_type_arm64ec,
	x64 = detail::load_config::chpe_arm64x_range_code_type_x64
};

template<typename... Bases>
class [[nodiscard]] chpe_arm64x_code_range_entry : public Bases...
{
public:
	using range_entry_type = packed_struct<detail::load_config::image_chpe_arm64x_range_entry>;

public:
	[[nodiscard]]
	inline range_entry_type& get_entry() noexcept;
	[[nodiscard]]
	inline const range_entry_type& get_entry() const noexcept;

	[[nodiscard]]
	chpe_arm64x_range_code_type get_code_type() const noexcept;
	void set_code_type(chpe_arm64x_range_code_type code_type) noexcept;

	[[nodiscard]]
	rva_type get_rva() const noexcept;
	void set_rva(rva_type rva) noexcept;

private:
	range_entry_type entry_;
};

template<typename... Bases>
class [[nodiscard]] chpe_arm64x_metadata_base : public Bases...
{
public:
	using range_entry_list_type = std::vector<chpe_arm64x_code_range_entry<Bases...>>;
	using metadata_type = packed_struct<detail::load_config::image_chpe_metadata_arm64x>;

public:
	[[nodiscard]]
	range_entry_list_type& get_range_entries() & noexcept;
	[[nodiscard]]
	const range_entry_list_type& get_range_entries() const& noexcept;
	[[nodiscard]]
	range_entry_list_type get_range_entries() && noexcept;

	[[nodiscard]]
	packed_struct<std::uint32_t>& get_version() noexcept;
	[[nodiscard]]
	const packed_struct<std::uint32_t>& get_version() const noexcept;

	[[nodiscard]]
	metadata_type& get_metadata() noexcept;
	[[nodiscard]]
	const metadata_type& get_metadata() const noexcept;

private:
	packed_struct<std::uint32_t> version_;
	metadata_type metadata_;
	range_entry_list_type range_entries_;
};

enum class chpe_x86_range_code_type : std::uint8_t
{
	arm64 = detail::load_config::chpe_x86_range_code_type_arm64,
	x86 = detail::load_config::chpe_x86_range_code_type_x86
};

template<typename... Bases>
class [[nodiscard]] chpe_x86_code_range_entry : public Bases...
{
public:
	using range_entry_type = packed_struct<detail::load_config::image_chpe_x86_range_entry>;

public:
	[[nodiscard]]
	inline range_entry_type& get_entry() noexcept;
	[[nodiscard]]
	inline const range_entry_type& get_entry() const noexcept;

	[[nodiscard]]
	chpe_x86_range_code_type get_code_type() const noexcept;
	void set_code_type(chpe_x86_range_code_type code_type) noexcept;

	[[nodiscard]]
	rva_type get_rva() const noexcept;
	void set_rva(rva_type rva) noexcept;

private:
	range_entry_type entry_;
};

template<typename... Bases>
class [[nodiscard]] chpe_x86_metadata_base : public Bases...
{
public:
	using range_entry_list_type = std::vector<chpe_x86_code_range_entry<Bases...>>;
	using metadata_type = packed_struct<detail::load_config::image_chpe_metadata_x86>;

public:
	[[nodiscard]]
	range_entry_list_type& get_range_entries() & noexcept;
	[[nodiscard]]
	const range_entry_list_type& get_range_entries() const& noexcept;
	[[nodiscard]]
	range_entry_list_type get_range_entries() && noexcept;

	[[nodiscard]]
	packed_struct<std::uint32_t>& get_version() noexcept;
	[[nodiscard]]
	const packed_struct<std::uint32_t>& get_version() const noexcept;

	[[nodiscard]]
	metadata_type& get_metadata() noexcept;
	[[nodiscard]]
	const metadata_type& get_metadata() const noexcept;

	[[nodiscard]]
	std::uint32_t get_metadata_size() const noexcept;

private:
	packed_struct<std::uint32_t> version_;
	metadata_type metadata_;
	range_entry_list_type range_entries_;
};

template<typename RelocationType>
class [[nodiscard]] dynamic_relocation_base
{
public:
	using relocation_type = packed_struct<RelocationType>;

public:
	static constexpr std::uint32_t max_page_reative_offset = 0xfffu;
	static constexpr std::uint32_t page_relative_offset_mask = 0xfffu;
	static constexpr bool is_scalar = std::is_scalar_v<RelocationType>;

public:
	//Relative offset from image_base_relocation RVA
	[[nodiscard]]
	std::uint32_t get_page_relative_offset() const noexcept
		requires (!is_scalar);
	[[nodiscard]]
	std::uint32_t get_page_relative_offset() const noexcept
		requires (is_scalar);

	void set_page_relative_offset(std::uint32_t offset)
		requires (!is_scalar);
	void set_page_relative_offset(std::uint32_t offset)
		requires (is_scalar);

public:
	[[nodiscard]]
	relocation_type& get_relocation() noexcept;
	[[nodiscard]]
	const relocation_type& get_relocation() const noexcept;

private:
	relocation_type relocation_;
};

class [[nodiscard]] import_control_transfer_dynamic_relocation
	: public dynamic_relocation_base<
	detail::load_config::image_import_control_transfer_dynamic_relocation>
{
public:
	static constexpr std::uint32_t max_iat_index = 0x7ffffu;

public:
	[[nodiscard]]
	inline bool is_indirect_call() const noexcept;

	[[nodiscard]]
	inline std::uint32_t get_iat_index() const noexcept;

	inline void set_indirect_call(bool is_indirect_call) noexcept;

	inline void set_iat_index(std::uint32_t iat_index);
};

class [[nodiscard]] indir_control_transfer_dynamic_relocation
	: public dynamic_relocation_base<
	detail::load_config::image_indir_control_transfer_dynamic_relocation>
{
public:
	[[nodiscard]]
	inline bool is_indirect_call() const noexcept;

	[[nodiscard]]
	inline bool is_rex_w_prefix() const noexcept;

	[[nodiscard]]
	inline bool is_cfg_check() const noexcept;

	inline void set_indirect_call(bool is_indirect_call) noexcept;

	inline void set_rex_w_prefix(bool is_indirect_call) noexcept;

	inline void set_cfg_check(bool is_indirect_call) noexcept;
};

class [[nodiscard]] switchtable_branch_dynamic_relocation
	: public dynamic_relocation_base<
	detail::load_config::image_switchtable_branch_dynamic_relocation>
{
public:
	enum class cpu_register : std::uint8_t
	{
		rax = 0,
		rcx = 1,
		rdx = 2,
		rbx = 3,
		rsp = 4,
		rbp = 5,
		rsi = 6,
		rdi = 7,
		r8 = 8,
		r9 = 9,
		r10 = 10,
		r11 = 11,
		r12 = 12,
		r13 = 13,
		r14 = 14,
		r15 = 15
	};

public:
	[[nodiscard]]
	inline cpu_register get_register() const noexcept;
	inline void set_register(cpu_register reg) noexcept;
};

class [[nodiscard]] arm64x_dynamic_relocation_base
	: public dynamic_relocation_base<detail::load_config::image_arm64x_dynamic_relocation>
{
public:
	enum class type : std::uint8_t
	{
		zero_fill = 0b00u,
		copy_data = 0b01u,
		add_delta = 0b10u
	};

public:
	[[nodiscard]]
	inline std::uint8_t get_meta() const noexcept;
	[[nodiscard]]
	inline type get_type() const noexcept;

	void set_meta(std::uint8_t meta);
};

class [[nodiscard]] arm64x_dynamic_relocation_sized_base
	: public arm64x_dynamic_relocation_base
{
public:
	static constexpr std::size_t max_size = 8;

public:
	[[nodiscard]]
	std::uint8_t get_size() const noexcept;

	void set_size(std::uint8_t size);
};

class [[nodiscard]] arm64x_dynamic_relocation_zero_fill
	: public arm64x_dynamic_relocation_sized_base
{
};

template<typename... Bases>
class [[nodiscard]] arm64x_dynamic_relocation_copy_data_base
	: public arm64x_dynamic_relocation_sized_base
	, public Bases...
{
public:
	using data_type = packed_byte_array<max_size>;

public:
	[[nodiscard]]
	data_type& get_data() noexcept;
	[[nodiscard]]
	const data_type& get_data() const noexcept;

private:
	data_type data_;
};

template<typename... Bases>
class [[nodiscard]] arm64x_dynamic_relocation_add_delta_base
	: public arm64x_dynamic_relocation_base
	, public Bases...
{
public:
	using value_type = packed_struct<std::uint16_t>;

	enum class multiplier : std::uint8_t
	{
		multiplier_8,
		multiplier_4
	};

	enum class sign : std::uint8_t
	{
		plus,
		minus
	};

public:
	[[nodiscard]]
	const value_type& get_value() const noexcept;
	[[nodiscard]]
	value_type& get_value() noexcept;

	[[nodiscard]]
	multiplier get_multiplier() const noexcept;

	[[nodiscard]]
	sign get_sign() const noexcept;

	void set_multiplier(multiplier value) noexcept;

	void set_sign(sign value) noexcept;

	[[nodiscard]]
	std::int32_t get_delta() const noexcept;

private:
	packed_struct<std::uint16_t> value_;
};

template<typename... Bases>
class [[nodiscard]] bdd_info : public Bases...
{
public:
	using descriptor_type = packed_struct<
		detail::load_config::image_bdd_info>;
	using dynamic_relocation_type = packed_struct<
		detail::load_config::image_bdd_dynamic_relocation>;
	using dynamic_relocation_list_type = std::vector<
		dynamic_relocation_type>;

public:
	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept;
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept;

	[[nodiscard]]
	const dynamic_relocation_list_type& get_relocations() const& noexcept;
	[[nodiscard]]
	dynamic_relocation_list_type& get_relocations() & noexcept;
	[[nodiscard]]
	dynamic_relocation_list_type get_relocations() && noexcept;

private:
	descriptor_type descriptor_;
	dynamic_relocation_list_type relocations_;
};

class [[nodiscard]] function_override_base_relocation
	: public dynamic_relocation_base<std::uint16_t>
{
public:
	enum class type
	{
		invalid = detail::load_config::image_function_override_type::invalid,
		x64_rel32 = detail::load_config::image_function_override_type::x64_rel32,
		arm64_branch26 = detail::load_config::image_function_override_type::arm64_branch26,
		arm64_thunk = detail::load_config::image_function_override_type::arm64_thunk
	};

public:
	[[nodiscard]]
	inline type get_type() const noexcept;

	inline void set_type(type type) noexcept;
};

template<typename Symbol, typename... Bases>
class [[nodiscard]] dynamic_relocation_list_base : public Bases...
{
public:
	using symbol_type = Symbol;
	using list_type = std::vector<symbol_type>;
	using base_relocation_type = packed_struct<detail::relocations::image_base_relocation>;

public:
	[[nodiscard]]
	const base_relocation_type& get_base_relocation() const noexcept;
	[[nodiscard]]
	base_relocation_type& get_base_relocation() noexcept;

	[[nodiscard]]
	const list_type& get_fixups() const& noexcept;
	[[nodiscard]]
	list_type& get_fixups() & noexcept;
	[[nodiscard]]
	list_type get_fixups() && noexcept;

private:
	base_relocation_type base_relocation_;
	list_type fixups_;
};

template<typename... Bases>
class [[nodiscard]] function_override_dynamic_relocation_item : public Bases...
{
public:
	using descriptor_type = packed_struct<
		detail::load_config::image_function_override_dynamic_relocation>;
	using rva_type = packed_struct<std::uint32_t>;
	using rva_list_type = std::vector<rva_type>;
	using base_relocation_list_base = dynamic_relocation_list_base<
		function_override_base_relocation, Bases...>;
	using base_relocation_list_type = std::vector<base_relocation_list_base>;
	using bdd_info_type = bdd_info<Bases...>;

public:
	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept;
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept;

	[[nodiscard]]
	const rva_list_type& get_rvas() const& noexcept;
	[[nodiscard]]
	rva_list_type& get_rvas() & noexcept;
	[[nodiscard]]
	rva_list_type get_rvas() && noexcept;

	[[nodiscard]]
	const base_relocation_list_type& get_relocations() const& noexcept;
	[[nodiscard]]
	base_relocation_list_type& get_relocations() & noexcept;
	[[nodiscard]]
	base_relocation_list_type get_relocations() && noexcept;

	[[nodiscard]]
	const bdd_info_type& get_bdd_info() const& noexcept;
	[[nodiscard]]
	bdd_info_type& get_bdd_info() & noexcept;
	[[nodiscard]]
	bdd_info_type get_bdd_info() && noexcept;

private:
	descriptor_type descriptor_;
	rva_list_type rvas_;
	base_relocation_list_type base_relocations_;
	bdd_info_type bdd_info_;
};

template<typename... Bases>
class [[nodiscard]] function_override_dynamic_relocation_base : public Bases...
{
public:
	using relocation_header_type = packed_struct<
		detail::load_config::image_function_override_header>;
	using item_list_type = std::vector<
		function_override_dynamic_relocation_item<Bases...>>;
	using base_relocation_type = packed_struct<detail::relocations::image_base_relocation>;

public:
	[[nodiscard]]
	const base_relocation_type& get_base_relocation() const noexcept;
	[[nodiscard]]
	base_relocation_type& get_base_relocation() noexcept;

	[[nodiscard]]
	const relocation_header_type& get_header() const noexcept;
	[[nodiscard]]
	relocation_header_type& get_header() noexcept;

	[[nodiscard]]
	const item_list_type& get_relocations() const& noexcept;
	[[nodiscard]]
	item_list_type& get_relocations() & noexcept;
	[[nodiscard]]
	item_list_type get_relocations() && noexcept;

private:
	relocation_header_type header_;
	item_list_type relocations_;
	base_relocation_type base_relocation_;
};

template<typename DynamicRelocation>
class [[nodiscard]] dynamic_relocation_table_common
{
public:
	using dynamic_relocation_type = packed_struct<DynamicRelocation>;

public:
	[[nodiscard]]
	const dynamic_relocation_type& get_dynamic_relocation() const noexcept;
	[[nodiscard]]
	dynamic_relocation_type& get_dynamic_relocation() noexcept;

public:
	[[nodiscard]]
	dynamic_relocation_symbol get_symbol() const noexcept;

private:
	dynamic_relocation_type dynamic_relocation_;
};

template<detail::executable_pointer Pointer, typename... Bases>
class [[nodiscard]] dynamic_relocation_table_base_v1
	: public dynamic_relocation_table_common<
		detail::load_config::image_dynamic_relocation<Pointer>>
	, public Bases...
{
public:
	using import_control_transfer_dynamic_fixup_list_type
		= dynamic_relocation_list_base<import_control_transfer_dynamic_relocation, Bases...>;
	using indir_control_transfer_dynamic_fixup_list_type
		= dynamic_relocation_list_base<indir_control_transfer_dynamic_relocation, Bases...>;
	using switchtable_branch_dynamic_fixup_list_type
		= dynamic_relocation_list_base<switchtable_branch_dynamic_relocation, Bases...>;
	using arm64x_dynamic_fixup_list_type
		= dynamic_relocation_list_base<std::variant<
		arm64x_dynamic_relocation_zero_fill,
		arm64x_dynamic_relocation_copy_data_base<Bases...>,
		arm64x_dynamic_relocation_add_delta_base<Bases...>>, Bases...>;

	using import_control_transfer_dynamic_relocation_list_type
		= std::vector<import_control_transfer_dynamic_fixup_list_type>;
	using indir_control_transfer_dynamic_relocation_list_type
		= std::vector<indir_control_transfer_dynamic_fixup_list_type>;
	using switchtable_branch_dynamic_relocation_list_type
		= std::vector<switchtable_branch_dynamic_fixup_list_type>;
	using arm64x_dynamic_relocation_list_type
		= std::vector<arm64x_dynamic_fixup_list_type>;
	using function_override_dynamic_relocation_list_type
		= std::vector<function_override_dynamic_relocation_base<Bases...>>;

	using fixup_list_type = std::variant<
		import_control_transfer_dynamic_relocation_list_type,
		indir_control_transfer_dynamic_relocation_list_type,
		switchtable_branch_dynamic_relocation_list_type,
		arm64x_dynamic_relocation_list_type,
		function_override_dynamic_relocation_list_type
	>;

public:
	[[nodiscard]]
	const fixup_list_type& get_fixup_lists() const& noexcept;
	[[nodiscard]]
	fixup_list_type& get_fixup_lists() & noexcept;
	[[nodiscard]]
	fixup_list_type get_fixup_lists() && noexcept;

private:
	fixup_list_type fixups_;
};

class [[nodiscard]] prologue_dynamic_relocation_header
{
public:
	using header_type = packed_struct<detail::load_config::image_prologue_dynamic_relocation_header>;
	using data_type = packed_byte_vector;

public:
	[[nodiscard]]
	inline const header_type& get_header() const noexcept;
	[[nodiscard]]
	inline header_type& get_header() noexcept;

	[[nodiscard]]
	inline const data_type& get_data() const& noexcept;
	[[nodiscard]]
	inline data_type& get_data() & noexcept;
	[[nodiscard]]
	inline data_type get_data() && noexcept;

private:
	header_type header_;
	data_type data_;
};

class [[nodiscard]] epilogue_branch_descriptor
{
public:
	using descriptor_type = packed_struct<std::uint16_t>;
	using value_type = packed_byte_vector;

public:
	[[nodiscard]]
	inline const descriptor_type& get_descriptor() const noexcept;
	[[nodiscard]]
	inline descriptor_type& get_descriptor() noexcept;

	[[nodiscard]]
	inline const value_type& get_value() const& noexcept;
	[[nodiscard]]
	inline value_type& get_value() & noexcept;
	[[nodiscard]]
	inline value_type get_value() && noexcept;

public:
	//Based on dumpbin output
	[[nodiscard]]
	inline std::uint8_t get_instr_size() const noexcept;
	[[nodiscard]]
	inline std::uint8_t get_disp_offset() const noexcept;
	[[nodiscard]]
	inline std::uint8_t get_disp_size() const noexcept;

	void set_instr_size(std::uint8_t instr_size);
	void set_disp_offset(std::uint8_t disp_offset);
	void set_disp_size(std::uint8_t disp_size);

private:
	descriptor_type descriptor_;
	value_type value_;
};

class [[nodiscard]] epilogue_branch_descriptor_bit_map
{
public:
	using data_type = packed_byte_vector;

public:
	[[nodiscard]]
	inline const data_type& get_data() const& noexcept;
	[[nodiscard]]
	inline data_type& get_data() & noexcept;
	[[nodiscard]]
	inline data_type get_data() && noexcept;

	[[nodiscard]]
	inline std::uint32_t get_bit_width() const noexcept;
	inline void set_bit_width(std::uint32_t bit_width) noexcept;

	[[nodiscard]]
	inline bit_stream<const data_type::vector_type> to_bit_stream() const
		noexcept(noexcept(bit_stream(data_.value())));
	[[nodiscard]]
	inline bit_stream<data_type::vector_type> to_bit_stream()
		noexcept(noexcept(bit_stream(data_.value())));

private:
	data_type data_;
	std::uint32_t bit_width_ = 0;
};

template<typename... Bases>
class [[nodiscard]] epilogue_dynamic_relocation_header_base
	: public Bases...
{
public:
	using header_type = packed_struct<detail::load_config::image_epilogue_dynamic_relocation_header>;
	//count of elements in this list is get_header()->branch_descriptor_count
	using epilogue_branch_descriptor_list_type = std::vector<epilogue_branch_descriptor>;

public:
	[[nodiscard]]
	const header_type& get_header() const noexcept;
	[[nodiscard]]
	header_type& get_header() noexcept;

	[[nodiscard]]
	const epilogue_branch_descriptor_list_type& get_branch_descriptors() const& noexcept;
	[[nodiscard]]
	epilogue_branch_descriptor_list_type& get_branch_descriptors() & noexcept;
	[[nodiscard]]
	epilogue_branch_descriptor_list_type get_branch_descriptors() && noexcept;

	//count of elements (branch index) in this bitmap is get_header()->epilogue_count
	[[nodiscard]]
	const epilogue_branch_descriptor_bit_map& get_branch_descriptor_bit_map() const noexcept;

	[[nodiscard]]
	epilogue_branch_descriptor_bit_map& get_branch_descriptor_bit_map() noexcept;

private:
	header_type header_;
	epilogue_branch_descriptor_list_type branch_descriptors_;
	epilogue_branch_descriptor_bit_map branch_descriptor_bit_map_;
};

template<detail::executable_pointer Pointer, typename... Bases>
class [[nodiscard]] dynamic_relocation_table_base_v2
	: public dynamic_relocation_table_common<detail::load_config::image_dynamic_relocation_v2<Pointer>>
	, public Bases...
{
public:
	using fixup_list_type = dynamic_relocation_list_base<dynamic_relocation_base<std::uint16_t>, Bases...>;
	using relocation_list_type = std::vector<fixup_list_type>;
	using header_type = std::variant<std::monostate, prologue_dynamic_relocation_header,
		epilogue_dynamic_relocation_header_base<Bases...>>;

public:
	[[nodiscard]]
	const relocation_list_type& get_fixup_lists() const& noexcept;
	[[nodiscard]]
	relocation_list_type& get_fixup_lists() & noexcept;
	[[nodiscard]]
	relocation_list_type get_fixup_lists() && noexcept;

	[[nodiscard]]
	const header_type& get_header() const& noexcept;
	[[nodiscard]]
	header_type& get_header() & noexcept;
	[[nodiscard]]
	header_type get_header() && noexcept;

private:
	relocation_list_type fixups_;
	header_type header_;
};

template<detail::executable_pointer Pointer, typename... Bases>
class [[nodiscard]] dynamic_relocation_table_base : public Bases...
{
public:
	using table_type = packed_struct<detail::load_config::image_dynamic_relocation_table>;
	using relocation_v1_list_type = std::vector<dynamic_relocation_table_base_v1<Pointer, Bases...>>;
	using relocation_v2_list_type = std::vector<dynamic_relocation_table_base_v2<Pointer, Bases...>>;
	using relocation_list_type = std::variant<std::monostate,
		relocation_v1_list_type, relocation_v2_list_type>;

public:
	[[nodiscard]]
	const table_type& get_table() const noexcept;
	[[nodiscard]]
	table_type& get_table() noexcept;
	
	[[nodiscard]]
	const relocation_list_type& get_relocations() const& noexcept;
	[[nodiscard]]
	relocation_list_type& get_relocations() & noexcept;
	[[nodiscard]]
	relocation_list_type get_relocations() && noexcept;

private:
	table_type table_;
	relocation_list_type relocations_;
};

enum class enclave_import_match : std::uint32_t
{
	none = detail::load_config::enclave_import_match::none,
	unique_id = detail::load_config::enclave_import_match::unique_id,
	author_id = detail::load_config::enclave_import_match::author_id,
	family_id = detail::load_config::enclave_import_match::family_id,
	image_id = detail::load_config::enclave_import_match::image_id
};

template<typename... Bases>
class [[nodiscard]] enclave_import_base
	: public Bases...
{
public:
	using descriptor_type = packed_struct<detail::load_config::image_enclave_import>;
	using extra_data_type = packed_byte_vector;

public:
	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept;
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept;

	[[nodiscard]]
	const packed_c_string& get_name() const& noexcept;
	[[nodiscard]]
	packed_c_string& get_name() & noexcept;
	[[nodiscard]]
	packed_c_string get_name() && noexcept;

	[[nodiscard]]
	const extra_data_type& get_extra_data() const& noexcept;
	[[nodiscard]]
	extra_data_type& get_extra_data() & noexcept;
	[[nodiscard]]
	extra_data_type get_extra_data() && noexcept;

public:
	[[nodiscard]]
	enclave_import_match get_match() const noexcept;
	void set_match(enclave_import_match match) noexcept;

private:
	descriptor_type descriptor_;
	packed_c_string name_;
	extra_data_type extra_data_;
};

struct enclave_policy_flags final : utilities::static_class
{
	enum value : std::uint32_t
	{
		debuggable = detail::load_config::image_enclave_policy_debuggable
	};
};

struct enclave_flags final : utilities::static_class
{
	enum value : std::uint32_t
	{
		primary_image = detail::load_config::image_enclave_flag_primary_image
	};
};

template<detail::executable_pointer Va, typename... Bases>
class [[nodiscard]] enclave_config_base
	: public Bases...
{
public:
	using descriptor_type = packed_struct<detail::load_config::image_enclave_config<Va>>;
	using enclave_import_list_type = std::vector<enclave_import_base<Bases...>>;
	using extra_data_type = packed_byte_vector;

public:
	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept;
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept;
	
	[[nodiscard]]
	const enclave_import_list_type& get_imports() const& noexcept;
	[[nodiscard]]
	enclave_import_list_type& get_imports() & noexcept;
	[[nodiscard]]
	enclave_import_list_type get_imports() && noexcept;

	[[nodiscard]]
	const extra_data_type& get_extra_data() const& noexcept;
	[[nodiscard]]
	extra_data_type& get_extra_data() & noexcept;
	[[nodiscard]]
	extra_data_type& get_extra_data() && noexcept;

public:
	[[nodiscard]]
	enclave_policy_flags::value get_policy_flags() const noexcept;
	void set_policy_flags(enclave_policy_flags::value flags) noexcept;
	
	[[nodiscard]]
	enclave_flags::value get_flags() const noexcept;
	void set_flags(enclave_flags::value flags) noexcept;

private:
	descriptor_type descriptor_;
	enclave_import_list_type import_list_;
	extra_data_type extra_data_;
};

template<typename... Bases>
class [[nodiscard]] volatile_metadata_base
	: public Bases...
{
public:
	using descriptor_type = packed_struct<detail::load_config::image_volatile_metadata>;
	using packed_rva_type = packed_struct<rva_type>;
	using range_entry_type = packed_struct<detail::load_config::range_table_entry>;
	using packed_rva_list_type = std::vector<packed_rva_type>;
	using range_entry_list_type = std::vector<range_entry_type>;

public:
	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept;
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept;

	[[nodiscard]]
	const packed_rva_list_type& get_access_rva_table() const& noexcept;
	[[nodiscard]]
	packed_rva_list_type& get_access_rva_table() & noexcept;
	[[nodiscard]]
	packed_rva_list_type get_access_rva_table() && noexcept;

	[[nodiscard]]
	const range_entry_list_type& get_range_table() const& noexcept;
	[[nodiscard]]
	range_entry_list_type& get_range_table() & noexcept;
	[[nodiscard]]
	range_entry_list_type get_range_table() && noexcept;

private:
	descriptor_type descriptor_;
	packed_rva_list_type access_rva_table_;
	range_entry_list_type range_table_;
};

template<typename Descriptor, typename... Bases>
class [[nodiscard]] load_config_directory_impl : public Bases...
{
public:
	using size_type = packed_struct<std::uint32_t>;
	using packed_descriptor_type = packed_struct<Descriptor>;
	using pointer_type = typename Descriptor::pointer_type;
	using packed_rva_type = packed_struct<rva_type>;
	using packed_rva_optional_list_type = std::optional<std::vector<packed_rva_type>>;

	using lock_prefix_table_type = std::optional<lock_prefix_table<pointer_type>>;
	using handler_table_type = std::optional<handler_table>;
	using guard_function_table_type = std::optional<std::vector<guard_function_base<Bases...>>>;
	using guard_address_taken_iat_entry_table_type = std::optional<std::vector<guard_function_common>>;
	using guard_long_jump_target_table_type = std::optional<std::vector<guard_function_common>>;
	using chpe_metadata_type = std::variant<std::monostate,
		chpe_x86_metadata_base<Bases...>, chpe_arm64x_metadata_base<Bases...>>;
	using dynamic_relocation_table_type = std::optional<dynamic_relocation_table_base<pointer_type, Bases...>>;
	using enclave_config_type = std::optional<enclave_config_base<pointer_type, Bases...>>;
	using volatile_metadata_type = std::optional<volatile_metadata_base<Bases...>>;

public:
	[[nodiscard]]
	version get_version() const noexcept;

	[[nodiscard]]
	bool version_exactly_matches() const noexcept;

	[[nodiscard]]
	const packed_descriptor_type& get_descriptor() const noexcept;

	[[nodiscard]]
	std::uint32_t get_descriptor_size() const noexcept;

	[[nodiscard]]
	size_type& get_size() noexcept;
	[[nodiscard]]
	const size_type& get_size() const noexcept;

public:
	[[nodiscard]]
	packed_descriptor_type& get_descriptor() noexcept;

	void set_version(version ver);

public:
	[[nodiscard]] lock_prefix_table_type& get_lock_prefix_table() & noexcept;
	[[nodiscard]] const lock_prefix_table_type& get_lock_prefix_table() const& noexcept;
	[[nodiscard]] lock_prefix_table_type get_lock_prefix_table() && noexcept;

	[[nodiscard]] global_flags::value get_global_flags_set() const noexcept;
	[[nodiscard]] global_flags::value get_global_flags_clear() const noexcept;
	void set_global_flags_set(global_flags::value flags) noexcept;
	void set_global_flags_clear(global_flags::value flags) noexcept;

	[[nodiscard]] process_heap_flags::value get_process_heap_flags() const noexcept;
	void set_process_heap_flags(process_heap_flags::value flags) noexcept;

	[[nodiscard]] dependent_load_flags::value get_dependent_load_flags() const noexcept;
	void set_dependent_load_flags(dependent_load_flags::value flags) noexcept;

public: //SafeSEH
	[[nodiscard]] handler_table_type& get_safeseh_handler_table() & noexcept;
	[[nodiscard]] const handler_table_type& get_safeseh_handler_table() const& noexcept;
	[[nodiscard]] handler_table_type get_safeseh_handler_table() && noexcept;

public: //GuardCF
	[[nodiscard]] guard_flags::value get_guard_flags() const noexcept;
	void set_guard_flags(guard_flags::value flags) noexcept;

	[[nodiscard]] std::uint8_t get_guard_cf_function_table_stride() const noexcept;
	void set_guard_cf_function_table_stride(std::uint8_t stride);
	
	[[nodiscard]] guard_function_table_type& get_guard_cf_function_table() & noexcept;
	[[nodiscard]] const guard_function_table_type& get_guard_cf_function_table() const& noexcept;
	[[nodiscard]] guard_function_table_type get_guard_cf_function_table() && noexcept;

public: //GuardCF extended
	[[nodiscard]] guard_address_taken_iat_entry_table_type&
		get_guard_address_taken_iat_entry_table() & noexcept;
	[[nodiscard]] const guard_address_taken_iat_entry_table_type&
		get_guard_address_taken_iat_entry_table() const& noexcept;
	[[nodiscard]] guard_address_taken_iat_entry_table_type
		get_guard_address_taken_iat_entry_table() && noexcept;

	[[nodiscard]] guard_long_jump_target_table_type&
		get_guard_long_jump_target_table() & noexcept;
	[[nodiscard]] const guard_long_jump_target_table_type&
		get_guard_long_jump_target_table() const& noexcept;
	[[nodiscard]] guard_long_jump_target_table_type
		get_guard_long_jump_target_table() && noexcept;

public: //HybridPE (CHPE)
	[[nodiscard]]
	chpe_metadata_type& get_chpe_metadata() & noexcept;
	[[nodiscard]]
	const chpe_metadata_type& get_chpe_metadata() const& noexcept;
	[[nodiscard]]
	chpe_metadata_type get_chpe_metadata() && noexcept;

public: //GuardRF, Retpoline
	[[nodiscard]] dynamic_relocation_table_type&
		get_dynamic_relocation_table() & noexcept;
	[[nodiscard]] const dynamic_relocation_table_type&
		get_dynamic_relocation_table() const& noexcept;
	[[nodiscard]] dynamic_relocation_table_type
		get_dynamic_relocation_table() && noexcept;

public: //Enclave
	[[nodiscard]] enclave_config_type& get_enclave_config() & noexcept;
	[[nodiscard]] const enclave_config_type& get_enclave_config() const& noexcept;
	[[nodiscard]] enclave_config_type get_enclave_config() && noexcept;

public: //Volatile metadata
	[[nodiscard]] volatile_metadata_type& get_volatile_metadata() & noexcept;
	[[nodiscard]] const volatile_metadata_type& get_volatile_metadata() const& noexcept;
	[[nodiscard]] volatile_metadata_type get_volatile_metadata() && noexcept;

public: //GuardEH
	[[nodiscard]] packed_rva_optional_list_type& get_eh_continuation_targets() & noexcept;
	[[nodiscard]] const packed_rva_optional_list_type& get_eh_continuation_targets() const& noexcept;
	[[nodiscard]] packed_rva_optional_list_type get_eh_continuation_targets() && noexcept;

private:
	size_type size_{};
	packed_descriptor_type descriptor_;
	lock_prefix_table_type lock_prefixes_;
	handler_table_type safeseh_handler_table_;
	guard_function_table_type guard_cf_function_table_;
	guard_address_taken_iat_entry_table_type guard_address_taken_iat_entry_table_;
	guard_long_jump_target_table_type guard_long_jump_target_table_;
	chpe_metadata_type chpe_metadata_;
	dynamic_relocation_table_type dynamic_relocation_table_;
	enclave_config_type enclave_config_;
	volatile_metadata_type volatile_metadata_;
	packed_rva_optional_list_type eh_continuation_targets_;
};

template<typename... Bases>
class [[nodiscard]] load_config_directory_base
{
public:
	using underlying_type32 = load_config_directory_impl<
		detail::load_config::image_load_config_directory32, Bases...>;
	using underlying_type64 = load_config_directory_impl<
		detail::load_config::image_load_config_directory64, Bases...>;
	using underlying_type = std::variant<underlying_type32, underlying_type64>;

public:
	explicit load_config_directory_base(bool is_64bit);

public:
	[[nodiscard]]
	underlying_type& get_value() & noexcept;
	[[nodiscard]]
	const underlying_type& get_value() const& noexcept;
	[[nodiscard]]
	underlying_type get_value() && noexcept;

private:
	static underlying_type create_underlying(bool is_64bit);

private:
	underlying_type value_;
};

using chpe_arm64x_metadata = chpe_arm64x_metadata_base<>;
using chpe_arm64x_metadata_details = chpe_arm64x_metadata_base<error_list>;
using chpe_x86_metadata = chpe_x86_metadata_base<>;
using chpe_x86_metadata_details = chpe_x86_metadata_base<error_list>;

template<detail::executable_pointer Pointer>
using dynamic_relocation_table = dynamic_relocation_table_base<Pointer>;
template<detail::executable_pointer Pointer>
using dynamic_relocation_table_details = dynamic_relocation_table_base<Pointer, error_list>;

template<detail::executable_pointer Pointer>
using dynamic_relocation_table_v1 = dynamic_relocation_table_base_v1<Pointer>;
template<detail::executable_pointer Pointer>
using dynamic_relocation_table_v1_details = dynamic_relocation_table_base_v1<Pointer, error_list>;

template<detail::executable_pointer Pointer>
using dynamic_relocation_table_v2 = dynamic_relocation_table_base_v2<Pointer>;
template<detail::executable_pointer Pointer>
using dynamic_relocation_table_v2_details = dynamic_relocation_table_base_v2<Pointer, error_list>;

template<typename Symbol>
using dynamic_relocation_list = dynamic_relocation_list_base<Symbol>;
template<typename Symbol>
using dynamic_relocation_list_details = dynamic_relocation_list_base<Symbol, error_list>;

using arm64x_dynamic_relocation_copy_data = arm64x_dynamic_relocation_copy_data_base<>;
using arm64x_dynamic_relocation_copy_data_details = arm64x_dynamic_relocation_copy_data_base<error_list>;
using arm64x_dynamic_relocation_add_delta = arm64x_dynamic_relocation_add_delta_base<>;
using arm64x_dynamic_relocation_add_delta_details = arm64x_dynamic_relocation_add_delta_base<error_list>;
using function_override_dynamic_relocation = function_override_dynamic_relocation_base<>;
using function_override_dynamic_relocation_details = function_override_dynamic_relocation_base<error_list>;

using epilogue_dynamic_relocation_header = epilogue_dynamic_relocation_header_base<>;
using epilogue_dynamic_relocation_header_details = epilogue_dynamic_relocation_header_base<error_list>;

template<detail::executable_pointer Pointer>
using enclave_config = enclave_config_base<Pointer>;
template<detail::executable_pointer Pointer>
using enclave_config_details = enclave_config_base<Pointer, error_list>;

using enclave_import = enclave_import_base<>;
using enclave_import_details = enclave_import_base<error_list>;

using volatile_metadata = volatile_metadata_base<>;
using volatile_metadata_details = volatile_metadata_base<error_list>;

using guard_function = guard_function_base<>;
using guard_function_details = guard_function_base<error_list>;

using load_config_directory = load_config_directory_base<>;
using load_config_directory_details = load_config_directory_base<error_list>;

} //namespace pe_bliss::load_config

#include "pe_bliss2/detail/load_config/load_config_directory-inl.h"
