#pragma once

namespace pe_bliss::load_config
{

template<detail::executable_pointer Pointer>
typename lock_prefix_table<Pointer>::lock_prefix_list_type& lock_prefix_table<
	Pointer>::get_prefix_va_list() & noexcept
{
	return prefixes_;
}

template<detail::executable_pointer Pointer>
const typename lock_prefix_table<Pointer>::lock_prefix_list_type& lock_prefix_table<
	Pointer>::get_prefix_va_list() const& noexcept
{
	return prefixes_;
}

template<detail::executable_pointer Pointer>
typename lock_prefix_table<Pointer>::lock_prefix_list_type lock_prefix_table<
	Pointer>::get_prefix_va_list() && noexcept
{
	return std::move(prefixes_);
}

inline handler_table::handler_list_type& handler_table
	::get_handler_list() & noexcept
{
	return handlers_;
}

inline const handler_table::handler_list_type& handler_table
	::get_handler_list() const& noexcept
{
	return handlers_;
}

inline handler_table::handler_list_type handler_table
	::get_handler_list() && noexcept
{
	return std::move(handlers_);
}

inline const guard_function_common::packed_rva_type& guard_function_common
	::get_rva() const noexcept
{
	return rva_;
}

inline guard_function_common::packed_rva_type& guard_function_common
	::get_rva() noexcept
{
	return rva_;
}

inline const guard_function_common::additional_data_type& guard_function_common
	::get_additional_data() const noexcept
{
	return flags_;
}

inline guard_function_common::additional_data_type& guard_function_common
	::get_additional_data() noexcept
{
	return flags_;
}

template<typename... Bases>
gfids_flags::value guard_function_base<Bases...>::get_flags() const noexcept
{
	return static_cast<gfids_flags::value>(get_additional_data()[0]);
}

template<typename... Bases>
void guard_function_base<Bases...>::set_flags(gfids_flags::value flags) noexcept
{
	get_additional_data()[0] = static_cast<std::byte>(flags);
}

template<typename... Bases>
const typename guard_function_base<Bases...>
	::optional_type_based_hash_type& guard_function_base<Bases...>
	::get_type_based_hash() const noexcept
{
	return type_based_hash_;
}

template<typename... Bases>
typename guard_function_base<Bases...>
	::optional_type_based_hash_type& guard_function_base<Bases...>
	::get_type_based_hash() noexcept
{
	return type_based_hash_;
}

template<typename... Bases>
typename chpe_arm64x_code_range_entry<Bases...>::range_entry_type&
	chpe_arm64x_code_range_entry<Bases...>::get_entry() noexcept
{
	return entry_;
}

template<typename... Bases>
const typename chpe_arm64x_code_range_entry<Bases...>::range_entry_type&
	chpe_arm64x_code_range_entry<Bases...>::get_entry() const noexcept
{
	return entry_;
}

template<typename... Bases>
chpe_arm64x_range_code_type chpe_arm64x_code_range_entry<Bases...>
::get_code_type() const noexcept
{
	return static_cast<chpe_arm64x_range_code_type>(entry_->start_offset
		& detail::load_config::chpe_arm64x_range_code_type_mask);
}

template<typename... Bases>
void chpe_arm64x_code_range_entry<Bases...>::set_code_type(
	chpe_arm64x_range_code_type code_type) noexcept
{
	entry_->start_offset &= ~detail::load_config::chpe_arm64x_range_code_type_mask;
	entry_->start_offset |= static_cast<std::uint8_t>(code_type);
}

template<typename... Bases>
rva_type chpe_arm64x_code_range_entry<Bases...>::get_rva() const noexcept
{
	return entry_->start_offset & ~detail::load_config::chpe_arm64x_range_code_type_mask;
}

template<typename... Bases>
void chpe_arm64x_code_range_entry<Bases...>::set_rva(rva_type rva) noexcept
{
	entry_->start_offset = (rva & ~detail::load_config::chpe_arm64x_range_code_type_mask)
		| static_cast<std::uint32_t>(get_code_type());
}

template<typename... Bases>
chpe_x86_range_code_type chpe_x86_code_range_entry<Bases...>
	::get_code_type() const noexcept
{
	return static_cast<chpe_x86_range_code_type>(entry_->start_offset
		& detail::load_config::chpe_x86_range_code_type_mask);
}

template<typename... Bases>
void chpe_x86_code_range_entry<Bases...>::set_code_type(
	chpe_x86_range_code_type code_type) noexcept
{
	entry_->start_offset &= ~detail::load_config::chpe_x86_range_code_type_mask;
	entry_->start_offset |= static_cast<std::uint8_t>(code_type);
}

template<typename... Bases>
rva_type chpe_x86_code_range_entry<Bases...>::get_rva() const noexcept
{
	return entry_->start_offset & ~detail::load_config::chpe_x86_range_code_type_mask;
}

template<typename... Bases>
void chpe_x86_code_range_entry<Bases...>::set_rva(rva_type rva) noexcept
{
	entry_->start_offset = (rva & ~detail::load_config::chpe_x86_range_code_type_mask)
		| static_cast<std::uint32_t>(get_code_type());
}

template<typename... Bases>
typename chpe_arm64x_metadata_base<Bases...>::range_entry_list_type&
	chpe_arm64x_metadata_base<Bases...>::get_range_entries() & noexcept
{
	return range_entries_;
}

template<typename... Bases>
const typename chpe_arm64x_metadata_base<Bases...>::range_entry_list_type&
	chpe_arm64x_metadata_base<Bases...>::get_range_entries() const& noexcept
{
	return range_entries_;
}

template<typename... Bases>
typename chpe_arm64x_metadata_base<Bases...>::range_entry_list_type
	chpe_arm64x_metadata_base<Bases...>::get_range_entries() && noexcept
{
	return std::move(range_entries_);
}

template<typename... Bases>
packed_struct<std::uint32_t>& chpe_arm64x_metadata_base<Bases...>::get_version() noexcept
{
	return version_;
}

template<typename... Bases>
const packed_struct<std::uint32_t>& chpe_arm64x_metadata_base<Bases...>::get_version() const noexcept
{
	return version_;
}

template<typename... Bases>
typename chpe_arm64x_metadata_base<Bases...>::metadata_type&
	chpe_arm64x_metadata_base<Bases...>::get_metadata() noexcept
{
	return metadata_;
}

template<typename... Bases>
const typename chpe_arm64x_metadata_base<Bases...>::metadata_type&
	chpe_arm64x_metadata_base<Bases...>::get_metadata() const noexcept
{
	return metadata_;
}

template<typename... Bases>
typename chpe_x86_code_range_entry<Bases...>::range_entry_type&
	chpe_x86_code_range_entry<Bases...>::get_entry() noexcept
{
	return entry_;
}

template<typename... Bases>
const typename chpe_x86_code_range_entry<Bases...>::range_entry_type&
	chpe_x86_code_range_entry<Bases...>::get_entry() const noexcept
{
	return entry_;
}

template<typename... Bases>
typename chpe_x86_metadata_base<Bases...>::range_entry_list_type&
	chpe_x86_metadata_base<Bases...>::get_range_entries() & noexcept
{
	return range_entries_;
}

template<typename... Bases>
const typename chpe_x86_metadata_base<Bases...>::range_entry_list_type&
	chpe_x86_metadata_base<Bases...>::get_range_entries() const& noexcept
{
	return range_entries_;
}

template<typename... Bases>
typename chpe_x86_metadata_base<Bases...>::range_entry_list_type
	chpe_x86_metadata_base<Bases...>::get_range_entries() && noexcept
{
	return std::move(range_entries_);
}

template<typename... Bases>
packed_struct<std::uint32_t>& chpe_x86_metadata_base<Bases...>
	::get_version() noexcept
{
	return version_;
}

template<typename... Bases>
const packed_struct<std::uint32_t>& chpe_x86_metadata_base<Bases...>
	::get_version() const noexcept
{
	return version_;
}

template<typename... Bases>
typename chpe_x86_metadata_base<Bases...>::metadata_type&
	chpe_x86_metadata_base<Bases...>::get_metadata() noexcept
{
	return metadata_;
}

template<typename... Bases>
const typename chpe_x86_metadata_base<Bases...>::metadata_type&
	chpe_x86_metadata_base<Bases...>::get_metadata() const noexcept
{
	return metadata_;
}

template<typename RelocationType>
std::uint32_t dynamic_relocation_base<RelocationType>
	::get_page_relative_offset() const noexcept
	requires (!is_scalar)
{
	return get_relocation()->metadata & page_relative_offset_mask;
}

template<typename RelocationType>
std::uint32_t dynamic_relocation_base<RelocationType>
	::get_page_relative_offset() const noexcept
	requires (is_scalar)
{
	return get_relocation().get() & page_relative_offset_mask;
}

template<typename RelocationType>
void dynamic_relocation_base<RelocationType>
	::set_page_relative_offset(std::uint32_t offset)
	requires (!is_scalar)
{
	if (offset > max_page_reative_offset)
		throw pe_error(load_config_errc::invalid_page_relative_offset);

	get_relocation()->metadata &= ~page_relative_offset_mask;
	get_relocation()->metadata |= (offset & page_relative_offset_mask);
}

template<typename RelocationType>
void dynamic_relocation_base<RelocationType>
	::set_page_relative_offset(std::uint32_t offset)
	requires (is_scalar)
{
	if (offset > max_page_reative_offset)
		throw pe_error(load_config_errc::invalid_page_relative_offset);

	get_relocation().get() &= ~page_relative_offset_mask;
	get_relocation().get() |= (offset & page_relative_offset_mask);
}

template<typename RelocationType>
typename dynamic_relocation_base<RelocationType>::relocation_type&
	dynamic_relocation_base<RelocationType>::get_relocation() noexcept
{
	return relocation_;
}

template<typename RelocationType>
const typename dynamic_relocation_base<RelocationType>::relocation_type&
	dynamic_relocation_base<RelocationType>::get_relocation() const noexcept
{
	return relocation_;
}

inline bool import_control_transfer_dynamic_relocation
	::is_indirect_call() const noexcept
{
	return static_cast<bool>(get_relocation()->metadata & 0x1000u);
}

inline std::uint32_t import_control_transfer_dynamic_relocation
	::get_iat_index() const noexcept
{
	return (get_relocation()->metadata & 0xffffe000u) >> 13u;
}

inline void import_control_transfer_dynamic_relocation::set_indirect_call(
	bool is_indirect_call) noexcept
{
	if (is_indirect_call)
		get_relocation()->metadata |= 0x1000u;
	else
		get_relocation()->metadata &= ~0x1000u;
}

inline void import_control_transfer_dynamic_relocation::set_iat_index(
	std::uint32_t iat_index)
{
	if (iat_index > max_iat_index)
		throw pe_error(load_config_errc::invalid_iat_index);

	get_relocation()->metadata &= ~0xffffe000u;
	get_relocation()->metadata |= iat_index << 13u;
}

inline bool indir_control_transfer_dynamic_relocation
	::is_indirect_call() const noexcept
{
	return static_cast<bool>(get_relocation()->metadata & 0x1000u);
}

inline bool indir_control_transfer_dynamic_relocation
	::is_rex_w_prefix() const noexcept
{
	return static_cast<bool>(get_relocation()->metadata & 0x2000u);
}

inline bool indir_control_transfer_dynamic_relocation
	::is_cfg_check() const noexcept
{
	return static_cast<bool>(get_relocation()->metadata & 0x4000u);
}

inline void indir_control_transfer_dynamic_relocation
	::set_indirect_call(bool is_indirect_call) noexcept
{
	if (is_indirect_call)
		get_relocation()->metadata |= 0x1000u;
	else
		get_relocation()->metadata &= ~0x1000u;
}

inline void indir_control_transfer_dynamic_relocation
	::set_rex_w_prefix(bool is_indirect_call) noexcept
{
	if (is_indirect_call)
		get_relocation()->metadata |= 0x2000u;
	else
		get_relocation()->metadata &= ~0x2000u;
}

inline void indir_control_transfer_dynamic_relocation
	::set_cfg_check(bool is_indirect_call) noexcept
{
	if (is_indirect_call)
		get_relocation()->metadata |= 0x4000u;
	else
		get_relocation()->metadata &= ~0x4000u;
}

inline switchtable_branch_dynamic_relocation::cpu_register
	switchtable_branch_dynamic_relocation::get_register() const noexcept
{
	return static_cast<cpu_register>((get_relocation()->metadata & 0xf000u) >> 12u);
}

inline void switchtable_branch_dynamic_relocation::set_register(
	cpu_register reg) noexcept
{
	get_relocation()->metadata &= ~0xf000u;
	get_relocation()->metadata |= (static_cast<std::uint8_t>(reg) & 0xfu) << 12u;
}

inline std::uint8_t arm64x_dynamic_relocation_base::get_meta() const noexcept
{
	return static_cast<std::uint8_t>((get_relocation()->metadata & 0xf000u) >> 12u);
}

inline arm64x_dynamic_relocation_base::type
	arm64x_dynamic_relocation_base::get_type() const noexcept
{
	return static_cast<type>(get_meta() & 0b11u);
}

template<typename... Bases>
typename arm64x_dynamic_relocation_copy_data_base<Bases...>::data_type&
	arm64x_dynamic_relocation_copy_data_base<Bases...>::get_data() noexcept
{
	return data_;
}

template<typename... Bases>
const typename arm64x_dynamic_relocation_copy_data_base<Bases...>::data_type&
	arm64x_dynamic_relocation_copy_data_base<Bases...>::get_data() const noexcept
{
	return data_;
}

template<typename... Bases>
const typename arm64x_dynamic_relocation_add_delta_base<Bases...>::value_type&
	arm64x_dynamic_relocation_add_delta_base<Bases...>::get_value() const noexcept
{
	return value_;
}

template<typename... Bases>
typename arm64x_dynamic_relocation_add_delta_base<Bases...>::value_type&
	arm64x_dynamic_relocation_add_delta_base<Bases...>::get_value() noexcept
{
	return value_;
}

template<typename... Bases>
const typename bdd_info<Bases...>::dynamic_relocation_list_type&
	bdd_info<Bases...>::get_relocations() const& noexcept
{
	return relocations_;
}

template<typename... Bases>
typename bdd_info<Bases...>::dynamic_relocation_list_type&
	bdd_info<Bases...>::get_relocations() & noexcept
{
	return relocations_;
}

template<typename... Bases>
typename bdd_info<Bases...>::dynamic_relocation_list_type
	bdd_info<Bases...>::get_relocations() && noexcept
{
	return std::move(relocations_);
}

template<typename... Bases>
const typename function_override_dynamic_relocation_item<Bases...>::rva_list_type&
	function_override_dynamic_relocation_item<Bases...>::get_rvas() const& noexcept
{
	return rvas_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_item<Bases...>::rva_list_type&
	function_override_dynamic_relocation_item<Bases...>::get_rvas() & noexcept
{
	return rvas_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_item<Bases...>::rva_list_type
	function_override_dynamic_relocation_item<Bases...>::get_rvas() && noexcept
{
	return std::move(rvas_);
}

template<typename... Bases>
const typename function_override_dynamic_relocation_item<Bases...>::base_relocation_list_type&
	function_override_dynamic_relocation_item<Bases...>::get_relocations() const& noexcept
{
	return base_relocations_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_item<Bases...>::base_relocation_list_type&
	function_override_dynamic_relocation_item<Bases...>::get_relocations() & noexcept
{
	return base_relocations_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_item<Bases...>::base_relocation_list_type
	function_override_dynamic_relocation_item<Bases...>::get_relocations() && noexcept
{
	return std::move(base_relocations_);
}

template<typename... Bases>
const typename function_override_dynamic_relocation_base<Bases...>::relocation_header_type&
	function_override_dynamic_relocation_base<Bases...>::get_header() const noexcept
{
	return header_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_base<Bases...>::relocation_header_type&
	function_override_dynamic_relocation_base<Bases...>::get_header() noexcept
{
	return header_;
}

template<typename... Bases>
const typename function_override_dynamic_relocation_base<Bases...>::item_list_type&
	function_override_dynamic_relocation_base<Bases...>::get_relocations() const& noexcept
{
	return relocations_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_base<Bases...>::item_list_type&
	function_override_dynamic_relocation_base<Bases...>::get_relocations() & noexcept
{
	return relocations_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_base<Bases...>::item_list_type
	function_override_dynamic_relocation_base<Bases...>::get_relocations() && noexcept
{
	return std::move(relocations_);
}

template<typename... Bases>
const typename function_override_dynamic_relocation_base<Bases...>::base_relocation_type&
	function_override_dynamic_relocation_base<Bases...>::get_base_relocation() const noexcept
{
	return base_relocation_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_base<Bases...>::base_relocation_type&
	function_override_dynamic_relocation_base<Bases...>::get_base_relocation() noexcept
{
	return base_relocation_;
}

template<typename... Bases>
const typename function_override_dynamic_relocation_item<Bases...>::bdd_info_type&
function_override_dynamic_relocation_item<Bases...>::get_bdd_info() const& noexcept
{
	return bdd_info_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_item<Bases...>::bdd_info_type&
function_override_dynamic_relocation_item<Bases...>::get_bdd_info() & noexcept
{
	return bdd_info_;
}

template<typename... Bases>
typename function_override_dynamic_relocation_item<Bases...>::bdd_info_type
function_override_dynamic_relocation_item<Bases...>::get_bdd_info() && noexcept
{
	return std::move(bdd_info_);
}

template<typename DynamicRelocation>
const typename dynamic_relocation_table_common<DynamicRelocation>
	::dynamic_relocation_type& dynamic_relocation_table_common<DynamicRelocation>
	::get_dynamic_relocation() const noexcept
{
	return dynamic_relocation_;
}

template<typename DynamicRelocation>
typename dynamic_relocation_table_common<DynamicRelocation>
	::dynamic_relocation_type& dynamic_relocation_table_common<DynamicRelocation>
	::get_dynamic_relocation() noexcept
{
	return dynamic_relocation_;
}

template<typename DynamicRelocation>
dynamic_relocation_symbol dynamic_relocation_table_common<DynamicRelocation>
	::get_symbol() const noexcept
{
	return static_cast<dynamic_relocation_symbol>(dynamic_relocation_->symbol);
}

template<typename Symbol, typename... Bases>
const typename dynamic_relocation_list_base<Symbol, Bases...>::base_relocation_type&
	dynamic_relocation_list_base<Symbol, Bases...>::get_base_relocation() const noexcept
{
	return base_relocation_;
}

template<typename Symbol, typename... Bases>
typename dynamic_relocation_list_base<Symbol, Bases...>::base_relocation_type&
	dynamic_relocation_list_base<Symbol, Bases...>::get_base_relocation() noexcept
{
	return base_relocation_;
}

template<typename Symbol, typename... Bases>
const typename dynamic_relocation_list_base<Symbol, Bases...>::list_type&
	dynamic_relocation_list_base<Symbol, Bases...>::get_fixups() const& noexcept
{
	return fixups_;
}

template<typename Symbol, typename... Bases>
typename dynamic_relocation_list_base<Symbol, Bases...>::list_type&
	dynamic_relocation_list_base<Symbol, Bases...>::get_fixups() & noexcept
{
	return fixups_;
}

template<typename Symbol, typename... Bases>
typename dynamic_relocation_list_base<Symbol, Bases...>::list_type
	dynamic_relocation_list_base<Symbol, Bases...>::get_fixups() && noexcept
{
	return std::move(fixups_);
}

template<detail::executable_pointer Pointer, typename... Bases>
const typename dynamic_relocation_table_base_v1<Pointer, Bases...>::fixup_list_type&
	dynamic_relocation_table_base_v1<Pointer, Bases...>::get_fixup_lists() const& noexcept
{
	return fixups_;
}

template<detail::executable_pointer Pointer, typename... Bases>
typename dynamic_relocation_table_base_v1<Pointer, Bases...>::fixup_list_type&
	dynamic_relocation_table_base_v1<Pointer, Bases...>::get_fixup_lists() & noexcept
{
	return fixups_;
}

template<detail::executable_pointer Pointer, typename... Bases>
typename dynamic_relocation_table_base_v1<Pointer, Bases...>::fixup_list_type
	dynamic_relocation_table_base_v1<Pointer, Bases...>::get_fixup_lists() && noexcept
{
	return std::move(fixups_);
}

inline const prologue_dynamic_relocation_header::header_type&
	prologue_dynamic_relocation_header::get_header() const noexcept
{
	return header_;
}

inline prologue_dynamic_relocation_header::header_type&
	prologue_dynamic_relocation_header::get_header() noexcept
{
	return header_;
}

inline const prologue_dynamic_relocation_header::data_type&
	prologue_dynamic_relocation_header::get_data() const& noexcept
{
	return data_;
}

inline prologue_dynamic_relocation_header::data_type&
	prologue_dynamic_relocation_header::get_data() & noexcept
{
	return data_;
}

inline prologue_dynamic_relocation_header::data_type
	prologue_dynamic_relocation_header::get_data() && noexcept
{
	return std::move(data_);
}

inline const epilogue_branch_descriptor::value_type&
	epilogue_branch_descriptor::get_value() const& noexcept
{
	return value_;
}

inline epilogue_branch_descriptor::value_type&
	epilogue_branch_descriptor::get_value() & noexcept
{
	return value_;
}

inline epilogue_branch_descriptor::value_type
	epilogue_branch_descriptor::get_value() && noexcept
{
	return std::move(value_);
}

inline std::uint8_t epilogue_branch_descriptor::get_instr_size() const noexcept
{
	return static_cast<std::uint8_t>(descriptor_.get() & 0xfu);
}

inline std::uint8_t epilogue_branch_descriptor::get_disp_offset() const noexcept
{
	return static_cast<std::uint8_t>((descriptor_.get() & 0xf0u) >> 4u);
}

inline std::uint8_t epilogue_branch_descriptor::get_disp_size() const noexcept
{
	return static_cast<std::uint8_t>((descriptor_.get() & 0xf00u) >> 8u);
}

inline const epilogue_branch_descriptor_bit_map::data_type&
	epilogue_branch_descriptor_bit_map::get_data() const& noexcept
{
	return data_;
}

inline epilogue_branch_descriptor_bit_map::data_type&
	epilogue_branch_descriptor_bit_map::get_data() & noexcept
{
	return data_;
}

inline epilogue_branch_descriptor_bit_map::data_type
	epilogue_branch_descriptor_bit_map::get_data() && noexcept
{
	return std::move(data_);
}

inline std::uint32_t epilogue_branch_descriptor_bit_map
	::get_bit_width() const noexcept
{
	return bit_width_;
}

inline void epilogue_branch_descriptor_bit_map
	::set_bit_width(std::uint32_t bit_width) noexcept
{
	bit_width_ = bit_width;
}

inline bit_stream<const epilogue_branch_descriptor_bit_map::data_type::vector_type>
	epilogue_branch_descriptor_bit_map::to_bit_stream() const
	noexcept(noexcept(bit_stream(data_.value())))
{
	return bit_stream(data_.value());
}

inline bit_stream<epilogue_branch_descriptor_bit_map::data_type::vector_type>
	epilogue_branch_descriptor_bit_map::to_bit_stream()
	noexcept(noexcept(bit_stream(data_.value())))
{
	return bit_stream(data_.value());
}

template<typename... Bases>
const typename epilogue_dynamic_relocation_header_base<Bases...>::header_type&
	epilogue_dynamic_relocation_header_base<Bases...>::get_header() const noexcept
{
	return header_;
}

template<typename... Bases>
typename epilogue_dynamic_relocation_header_base<Bases...>::header_type&
	epilogue_dynamic_relocation_header_base<Bases...>::get_header() noexcept
{
	return header_;
}

template<typename... Bases>
const typename epilogue_dynamic_relocation_header_base<Bases...>::epilogue_branch_descriptor_list_type&
	epilogue_dynamic_relocation_header_base<Bases...>::get_branch_descriptors() const& noexcept
{
	return branch_descriptors_;
}

template<typename... Bases>
typename epilogue_dynamic_relocation_header_base<Bases...>::epilogue_branch_descriptor_list_type&
	epilogue_dynamic_relocation_header_base<Bases...>::get_branch_descriptors() & noexcept
{
	return branch_descriptors_;
}

template<typename... Bases>
typename epilogue_dynamic_relocation_header_base<Bases...>::epilogue_branch_descriptor_list_type
	epilogue_dynamic_relocation_header_base<Bases...>::get_branch_descriptors() && noexcept
{
	return std::move(branch_descriptors_);
}

template<typename... Bases>
const epilogue_branch_descriptor_bit_map& epilogue_dynamic_relocation_header_base<
	Bases...>::get_branch_descriptor_bit_map() const noexcept
{
	return branch_descriptor_bit_map_;
}

template<typename... Bases>
epilogue_branch_descriptor_bit_map& epilogue_dynamic_relocation_header_base<
	Bases...>::get_branch_descriptor_bit_map() noexcept
{
	return branch_descriptor_bit_map_;
}

template<detail::executable_pointer Pointer, typename... Bases>
const typename dynamic_relocation_table_base_v2<Pointer, Bases...>::relocation_list_type&
	dynamic_relocation_table_base_v2<Pointer, Bases...>::get_fixup_lists() const& noexcept
{
	return fixups_;
}

template<detail::executable_pointer Pointer, typename... Bases>
typename dynamic_relocation_table_base_v2<Pointer, Bases...>::relocation_list_type&
	dynamic_relocation_table_base_v2<Pointer, Bases...>::get_fixup_lists() & noexcept
{
	return fixups_;
}

template<detail::executable_pointer Pointer, typename... Bases>
typename dynamic_relocation_table_base_v2<Pointer, Bases...>::relocation_list_type
	dynamic_relocation_table_base_v2<Pointer, Bases...>::get_fixup_lists() && noexcept
{
	return std::move(fixups_);
}

template<detail::executable_pointer Pointer, typename... Bases>
const typename dynamic_relocation_table_base_v2<Pointer, Bases...>::header_type&
	dynamic_relocation_table_base_v2<Pointer, Bases...>::get_header() const& noexcept
{
	return header_;
}

template<detail::executable_pointer Pointer, typename... Bases>
typename dynamic_relocation_table_base_v2<Pointer, Bases...>::header_type&
	dynamic_relocation_table_base_v2<Pointer, Bases...>::get_header() & noexcept
{
	return header_;
}

template<detail::executable_pointer Pointer, typename... Bases>
typename dynamic_relocation_table_base_v2<Pointer, Bases...>::header_type
	dynamic_relocation_table_base_v2<Pointer, Bases...>::get_header() && noexcept
{
	return std::move(header_);
}

template<detail::executable_pointer Pointer, typename... Bases>
const typename dynamic_relocation_table_base<Pointer, Bases...>::table_type&
	dynamic_relocation_table_base<Pointer, Bases...>::get_table() const noexcept
{
	return table_;
}

template<detail::executable_pointer Pointer, typename... Bases>
typename dynamic_relocation_table_base<Pointer, Bases...>::table_type&
	dynamic_relocation_table_base<Pointer, Bases...>::get_table() noexcept
{
	return table_;
}

template<detail::executable_pointer Pointer, typename... Bases>
const typename dynamic_relocation_table_base<Pointer, Bases...>::relocation_list_type&
	dynamic_relocation_table_base<Pointer, Bases...>::get_relocations() const& noexcept
{
	return relocations_;
}

template<detail::executable_pointer Pointer, typename... Bases>
typename dynamic_relocation_table_base<Pointer, Bases...>::relocation_list_type&
	dynamic_relocation_table_base<Pointer, Bases...>::get_relocations() & noexcept
{
	return relocations_;
}

template<detail::executable_pointer Pointer, typename... Bases>
typename dynamic_relocation_table_base<Pointer, Bases...>::relocation_list_type
	dynamic_relocation_table_base<Pointer, Bases...>::get_relocations() && noexcept
{
	return std::move(relocations_);
}

template<typename... Bases>
const packed_c_string& enclave_import_base<Bases...>::get_name() const& noexcept
{
	return name_;
}

template<typename... Bases>
packed_c_string& enclave_import_base<Bases...>::get_name() & noexcept
{
	return name_;
}

template<typename... Bases>
packed_c_string enclave_import_base<Bases...>::get_name() && noexcept
{
	return std::move(name_);
}

template<typename... Bases>
const typename enclave_import_base<Bases...>::extra_data_type&
	enclave_import_base<Bases...>::get_extra_data() const& noexcept
{
	return extra_data_;
}

template<typename... Bases>
typename enclave_import_base<Bases...>::extra_data_type&
	enclave_import_base<Bases...>::get_extra_data() & noexcept
{
	return extra_data_;
}

template<typename... Bases>
typename enclave_import_base<Bases...>::extra_data_type
	enclave_import_base<Bases...>::get_extra_data() && noexcept
{
	return std::move(extra_data_);
}

template<typename... Bases>
typename enclave_import_match enclave_import_base<Bases...>::get_match() const noexcept
{
	return static_cast<enclave_import_match>(this->descriptor_->match_type);
}

template<typename... Bases>
void enclave_import_base<Bases...>::set_match(enclave_import_match match) noexcept
{
	this->descriptor_->match_type = static_cast<std::uint32_t>(match);
}

template<detail::executable_pointer Va, typename... Bases>
const typename enclave_config_base<Va, Bases...>::enclave_import_list_type&
	enclave_config_base<Va, Bases...>::get_imports() const& noexcept
{
	return import_list_;
}

template<detail::executable_pointer Va, typename... Bases>
typename enclave_config_base<Va, Bases...>::enclave_import_list_type&
	enclave_config_base<Va, Bases...>::get_imports() & noexcept
{
	return import_list_;
}

template<detail::executable_pointer Va, typename... Bases>
typename enclave_config_base<Va, Bases...>::enclave_import_list_type
	enclave_config_base<Va, Bases...>::get_imports() && noexcept
{
	return std::move(import_list_);
}

template<detail::executable_pointer Va, typename... Bases>
const typename enclave_config_base<Va, Bases...>::extra_data_type&
	enclave_config_base<Va, Bases...>::get_extra_data() const& noexcept
{
	return extra_data_;
}

template<detail::executable_pointer Va, typename... Bases>
typename enclave_config_base<Va, Bases...>::extra_data_type&
	enclave_config_base<Va, Bases...>::get_extra_data() & noexcept
{
	return extra_data_;
}

template<detail::executable_pointer Va, typename... Bases>
typename enclave_config_base<Va, Bases...>::extra_data_type&
	enclave_config_base<Va, Bases...>::get_extra_data() && noexcept
{
	return std::move(extra_data_);
}

template<detail::executable_pointer Va, typename... Bases>
enclave_policy_flags::value enclave_config_base<Va, Bases...>
	::get_policy_flags() const noexcept
{
	return static_cast<enclave_policy_flags::value>(this->descriptor_->policy_flags);
}

template<detail::executable_pointer Va, typename... Bases>
void enclave_config_base<Va, Bases...>::set_policy_flags(
	enclave_policy_flags::value flags) noexcept
{
	this->descriptor_->policy_flags = flags;
}

template<detail::executable_pointer Va, typename... Bases>
enclave_flags::value enclave_config_base<Va, Bases...>::get_flags() const noexcept
{
	return static_cast<enclave_flags::value>(this->descriptor_->enclave_flags);
}

template<detail::executable_pointer Va, typename... Bases>
void enclave_config_base<Va, Bases...>::set_flags(enclave_flags::value flags) noexcept
{
	this->descriptor_->enclave_flags = flags;
}

template<typename... Bases>
const typename volatile_metadata_base<Bases...>::packed_rva_list_type&
	volatile_metadata_base<Bases...>::get_access_rva_table() const& noexcept
{
	return access_rva_table_;
}

template<typename... Bases>
typename volatile_metadata_base<Bases...>::packed_rva_list_type&
	volatile_metadata_base<Bases...>::get_access_rva_table() & noexcept
{
	return access_rva_table_;
}

template<typename... Bases>
typename volatile_metadata_base<Bases...>::packed_rva_list_type
	volatile_metadata_base<Bases...>::get_access_rva_table() && noexcept
{
	return std::move(access_rva_table_);
}

template<typename... Bases>
const typename volatile_metadata_base<Bases...>::range_entry_list_type&
	volatile_metadata_base<Bases...>::get_range_table() const& noexcept
{
	return range_table_;
}

template<typename... Bases>
typename volatile_metadata_base<Bases...>::range_entry_list_type&
	volatile_metadata_base<Bases...>::get_range_table() & noexcept
{
	return range_table_;
}

template<typename... Bases>
typename volatile_metadata_base<Bases...>::range_entry_list_type
	volatile_metadata_base<Bases...>::get_range_table() && noexcept
{
	return std::move(range_table_);
}

template<typename Descriptor, typename... Bases>
std::uint32_t load_config_directory_impl<Descriptor, Bases...>
	::get_descriptor_size() const noexcept
{
	if (size_.get() < size_.packed_size)
		return 0u;

	return size_.get() - size_.packed_size;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::size_type&
	load_config_directory_impl<Descriptor, Bases...>::get_size() noexcept
{
	return size_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>::size_type&
	load_config_directory_impl<Descriptor, Bases...>::get_size() const noexcept
{
	return size_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::lock_prefix_table_type&
	load_config_directory_impl<Descriptor, Bases...>::get_lock_prefix_table() & noexcept
{
	return lock_prefixes_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>::lock_prefix_table_type&
	load_config_directory_impl<Descriptor, Bases...>::get_lock_prefix_table() const& noexcept
{
	return lock_prefixes_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::lock_prefix_table_type
	load_config_directory_impl<Descriptor, Bases...>::get_lock_prefix_table() && noexcept
{
	return std::move(lock_prefixes_);
}

template<typename Descriptor, typename... Bases>
global_flags::value load_config_directory_impl<Descriptor, Bases...>
	::get_global_flags_set() const noexcept
{
	return static_cast<global_flags::value>(this->descriptor_->base.global_flags_set);
}

template<typename Descriptor, typename... Bases>
global_flags::value load_config_directory_impl<Descriptor, Bases...>
	::get_global_flags_clear() const noexcept
{
	return static_cast<global_flags::value>(this->descriptor_->base.global_flags_clear);
}

template<typename Descriptor, typename... Bases>
void load_config_directory_impl<Descriptor, Bases...>::set_global_flags_set(
	global_flags::value flags) noexcept
{
	this->descriptor_->base.global_flags_set = flags;
}

template<typename Descriptor, typename... Bases>
void load_config_directory_impl<Descriptor, Bases...>::set_global_flags_clear(
	global_flags::value flags) noexcept
{
	this->descriptor_->base.global_flags_clear = flags;
}

template<typename Descriptor, typename... Bases>
process_heap_flags::value load_config_directory_impl<Descriptor, Bases...>
	::get_process_heap_flags() const noexcept
{
	return static_cast<process_heap_flags::value>(this->descriptor_->base.process_heap_flags);
}

template<typename Descriptor, typename... Bases>
void load_config_directory_impl<Descriptor, Bases...>::set_process_heap_flags(
	process_heap_flags::value flags) noexcept
{
	this->descriptor_->base.process_heap_flags = flags;
}

template<typename Descriptor, typename... Bases>
dependent_load_flags::value load_config_directory_impl<Descriptor, Bases...>
	::get_dependent_load_flags() const noexcept
{
	return static_cast<dependent_load_flags::value>(
		this->descriptor_->base.dependent_load_flags);
}

template<typename Descriptor, typename... Bases>
void load_config_directory_impl<Descriptor, Bases...>::set_dependent_load_flags(
	dependent_load_flags::value flags) noexcept
{
	this->descriptor_->base.dependent_load_flags = flags;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::handler_table_type&
	load_config_directory_impl<Descriptor, Bases...>::get_safeseh_handler_table() & noexcept
{
	return safeseh_handler_table_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>::handler_table_type&
	load_config_directory_impl<Descriptor, Bases...>::get_safeseh_handler_table() const& noexcept
{
	return safeseh_handler_table_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::handler_table_type
	load_config_directory_impl<Descriptor, Bases...>::get_safeseh_handler_table() && noexcept
{
	return std::move(safeseh_handler_table_);
}

template<typename Descriptor, typename... Bases>
guard_flags::value load_config_directory_impl<Descriptor, Bases...>
	::get_guard_flags() const noexcept
{
	return static_cast<guard_flags::value>(this->descriptor_->cf_guard.guard_flags);
}

template<typename Descriptor, typename... Bases>
void load_config_directory_impl<Descriptor, Bases...>::set_guard_flags(
	guard_flags::value flags) noexcept
{
	this->descriptor_->cf_guard.guard_flags = flags;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>
	::guard_function_table_type& load_config_directory_impl<Descriptor, Bases...>
	::get_guard_cf_function_table() & noexcept
{
	return guard_cf_function_table_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>
	::guard_function_table_type& load_config_directory_impl<Descriptor, Bases...>
	::get_guard_cf_function_table() const& noexcept
{
	return guard_cf_function_table_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>
	::guard_function_table_type load_config_directory_impl<Descriptor, Bases...>
	::get_guard_cf_function_table() && noexcept
{
	return std::move(guard_cf_function_table_);
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>
	::guard_address_taken_iat_entry_table_type& load_config_directory_impl<Descriptor, Bases...>
	::get_guard_address_taken_iat_entry_table() & noexcept
{
	return guard_address_taken_iat_entry_table_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>
	::guard_address_taken_iat_entry_table_type& load_config_directory_impl<Descriptor, Bases...>
	::get_guard_address_taken_iat_entry_table() const& noexcept
{
	return guard_address_taken_iat_entry_table_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>
	::guard_address_taken_iat_entry_table_type load_config_directory_impl<Descriptor, Bases...>
	::get_guard_address_taken_iat_entry_table() && noexcept
{
	return std::move(guard_address_taken_iat_entry_table_);
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>
 ::guard_long_jump_target_table_type& load_config_directory_impl<Descriptor, Bases...>
	::get_guard_long_jump_target_table() & noexcept
{
	return guard_long_jump_target_table_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>
	::guard_long_jump_target_table_type& load_config_directory_impl<Descriptor, Bases...>
	::get_guard_long_jump_target_table() const& noexcept
{
	return guard_long_jump_target_table_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>
	::guard_long_jump_target_table_type load_config_directory_impl<Descriptor, Bases...>
	::get_guard_long_jump_target_table() && noexcept
{
	return std::move(guard_long_jump_target_table_);
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::chpe_metadata_type&
	load_config_directory_impl<Descriptor, Bases...>::get_chpe_metadata() & noexcept
{
	return chpe_metadata_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>::chpe_metadata_type&
	load_config_directory_impl<Descriptor, Bases...>::get_chpe_metadata() const& noexcept
{
	return chpe_metadata_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::chpe_metadata_type
	load_config_directory_impl<Descriptor, Bases...>::get_chpe_metadata() && noexcept
{
	return std::move(chpe_metadata_);
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::dynamic_relocation_table_type&
	load_config_directory_impl<Descriptor, Bases...>::get_dynamic_relocation_table() & noexcept
{
	return dynamic_relocation_table_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>::dynamic_relocation_table_type&
	load_config_directory_impl<Descriptor, Bases...>::get_dynamic_relocation_table() const& noexcept
{
	return dynamic_relocation_table_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::dynamic_relocation_table_type
	load_config_directory_impl<Descriptor, Bases...>::get_dynamic_relocation_table() && noexcept
{
	return std::move(dynamic_relocation_table_);
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::enclave_config_type&
	load_config_directory_impl<Descriptor, Bases...>::get_enclave_config() & noexcept
{
	return enclave_config_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>::enclave_config_type&
	load_config_directory_impl<Descriptor, Bases...>::get_enclave_config() const& noexcept
{
	return enclave_config_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::enclave_config_type
	load_config_directory_impl<Descriptor, Bases...>::get_enclave_config() && noexcept
{
	return std::move(enclave_config_);
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::volatile_metadata_type&
	load_config_directory_impl<Descriptor, Bases...>::get_volatile_metadata() & noexcept
{
	return volatile_metadata_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>::volatile_metadata_type&
	load_config_directory_impl<Descriptor, Bases...>::get_volatile_metadata() const& noexcept
{
	return volatile_metadata_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::volatile_metadata_type
	load_config_directory_impl<Descriptor, Bases...>::get_volatile_metadata() && noexcept
{
	return std::move(volatile_metadata_);
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::packed_rva_optional_list_type&
	load_config_directory_impl<Descriptor, Bases...>::get_eh_continuation_targets() & noexcept
{
	return eh_continuation_targets_;
}

template<typename Descriptor, typename... Bases>
const typename load_config_directory_impl<Descriptor, Bases...>::packed_rva_optional_list_type&
	load_config_directory_impl<Descriptor, Bases...>::get_eh_continuation_targets() const& noexcept
{
	return eh_continuation_targets_;
}

template<typename Descriptor, typename... Bases>
typename load_config_directory_impl<Descriptor, Bases...>::packed_rva_optional_list_type
	load_config_directory_impl<Descriptor, Bases...>::get_eh_continuation_targets() && noexcept
{
	return std::move(eh_continuation_targets_);
}

template<typename... Bases>
load_config_directory_base<Bases...>::load_config_directory_base(bool is_64bit)
	:value_(create_underlying(is_64bit))
{
}

template<typename... Bases>
typename load_config_directory_base<Bases...>::underlying_type&
	load_config_directory_base<Bases...>::get_value() & noexcept
{
	return value_;
}

template<typename... Bases>
const typename load_config_directory_base<Bases...>::underlying_type&
	load_config_directory_base<Bases...>::get_value() const& noexcept
{
	return value_;
}

template<typename... Bases>
typename load_config_directory_base<Bases...>::underlying_type
	load_config_directory_base<Bases...>::get_value() && noexcept
{
	return std::move(value_);
}

template<typename... Bases>
typename load_config_directory_base<Bases...>::underlying_type
	load_config_directory_base<Bases...>::create_underlying(bool is_64bit)
{
	return is_64bit
		? underlying_type(std::in_place_type<underlying_type64>)
		: underlying_type(std::in_place_type<underlying_type32>);
}

inline function_override_base_relocation::type function_override_base_relocation
	::get_type() const noexcept
{
	return static_cast<function_override_base_relocation::type>(
		(get_relocation().get() >> 12u) & 0xfu);
}

inline void function_override_base_relocation::set_type(
	function_override_base_relocation::type type) noexcept
{
	get_relocation().get() &= ~0xf000u;
	get_relocation().get() |= (static_cast<std::uint32_t>(type) & 0xfu) << 12u;
}

} //namespace pe_bliss::load_config
