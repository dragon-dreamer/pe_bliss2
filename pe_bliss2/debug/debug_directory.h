#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/detail/debug/image_debug_directory.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_byte_vector.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/packed_c_string.h"

namespace buffers
{
class input_buffer_stateful_wrapper;
} //namespace buffers

namespace pe_bliss::debug
{

enum class debug_directory_errc
{
	unsupported_type = 1,
	unable_to_deserialize,
	too_long_hash,
	unable_to_deserialize_hash,
	unable_to_deserialize_name,
	unable_to_deserialize_symbols,
	unaligned_data_size,
	too_many_symbols,
	too_many_entries,
	number_of_symbols_mismatch,
	pointer_to_symbol_table_mismatch,
	virtual_coff_string_table
};

std::error_code make_error_code(debug_directory_errc) noexcept;

enum class coff_symbol_type : std::uint8_t
{
	sym_null = detail::debug::coff_type_null,
	sym_void = detail::debug::coff_type_void,
	sym_char = detail::debug::coff_type_char,
	sym_short = detail::debug::coff_type_short,
	sym_int = detail::debug::coff_type_int,
	sym_long = detail::debug::coff_type_long,
	sym_float = detail::debug::coff_type_float,
	sym_double = detail::debug::coff_type_double,
	sym_struct = detail::debug::coff_type_struct,
	sym_union = detail::debug::coff_type_union,
	sym_enum = detail::debug::coff_type_enum,
	sym_member_of_enum = detail::debug::coff_type_member_of_enum,
	sym_uchar = detail::debug::coff_type_uchar,
	sym_ushort = detail::debug::coff_type_ushort,
	sym_uint = detail::debug::coff_type_uint,
	sym_ulong = detail::debug::coff_type_ulong,
	sym_long_double = detail::debug::coff_type_long_double
};

enum class coff_symbol_type_attr : std::uint16_t
{
	no_derived = detail::debug::coff_type_no_derived,
	pointer = detail::debug::coff_type_pointer,
	function_return_value = detail::debug::coff_type_function_return_value,
	array = detail::debug::coff_type_array
};

enum class coff_storage_class : std::uint8_t
{
	null = detail::debug::coff_storage_class_null,
	automatic = detail::debug::coff_storage_class_automatic,
	external = detail::debug::coff_storage_class_external,
	static_class = detail::debug::coff_storage_class_static,
	register_class = detail::debug::coff_storage_class_register,
	external_definition = detail::debug::coff_storage_class_external_definition,
	label = detail::debug::coff_storage_class_label,
	undefined_label = detail::debug::coff_storage_class_undefined_label,
	member_of_structure = detail::debug::coff_storage_class_member_of_structure,
	function_argument = detail::debug::coff_storage_class_function_argument,
	structure_tag = detail::debug::coff_storage_class_structure_tag,
	member_of_union = detail::debug::coff_storage_class_member_of_union,
	union_tag = detail::debug::coff_storage_class_union_tag,
	type_definition = detail::debug::coff_storage_class_type_definition,
	undefined_static = detail::debug::coff_storage_class_undefined_static,
	enumeration_tag = detail::debug::coff_storage_class_enumeration_tag,
	member_of_enumeration = detail::debug::coff_storage_class_member_of_enumeration,
	register_param = detail::debug::coff_storage_class_register_param,
	bit_field = detail::debug::coff_storage_class_bit_field,
	automatic_argument = detail::debug::coff_storage_class_automatic_argument,
	dummy_entry = detail::debug::coff_storage_class_dummy_entry,
	begin_or_end_of_block = detail::debug::coff_storage_class_begin_or_end_of_block,
	begin_or_end_of_function = detail::debug::coff_storage_class_begin_or_end_of_function,
	end_of_structure = detail::debug::coff_storage_class_end_of_structure,
	file_name = detail::debug::coff_storage_class_file_name,
	line_number = detail::debug::coff_storage_class_line_number,
	duplicate_tag = detail::debug::coff_storage_class_duplicate_tag,
	hidden = detail::debug::coff_storage_class_hidden,
	physical_end_of_function = detail::debug::coff_storage_class_physical_end_of_function
};

class [[nodiscard]] coff_symbol
	: public detail::packed_struct_base<detail::debug::coff_symbol>
{
public:
	using name_type = std::variant<std::string, packed_c_string>;

	static constexpr std::int16_t section_number_undef
		= detail::debug::coff_section_number_undef;
	static constexpr std::int16_t section_number_absolute
		= detail::debug::coff_section_number_absolute;
	static constexpr std::int16_t section_number_debug
		= detail::debug::coff_section_number_debug;

public:
	[[nodiscard]]
	name_type& get_name() & noexcept
	{
		return name_;
	}

	[[nodiscard]]
	const name_type& get_name() const& noexcept
	{
		return name_;
	}

	[[nodiscard]]
	name_type get_name() && noexcept
	{
		return std::move(name_);
	}

	[[nodiscard]]
	coff_symbol_type get_type() const noexcept
	{
		if (descriptor_->type == detail::debug::coff_type_long_double)
			return coff_symbol_type::sym_long_double;
		return static_cast<coff_symbol_type>(
			descriptor_->type & detail::debug::coff_type_mask);
	}

	[[nodiscard]]
	coff_symbol_type_attr get_type_attr() const noexcept
	{
		if (descriptor_->type == detail::debug::coff_type_long_double)
			return coff_symbol_type_attr::no_derived;
		return static_cast<coff_symbol_type_attr>(
			descriptor_->type & detail::debug::coff_type_attr_mask);
	}

	[[nodiscard]]
	coff_storage_class get_storage_class() const noexcept
	{
		return static_cast<coff_storage_class>(
			descriptor_->storage_class);
	}

private:
	name_type name_;
};

template<typename... Bases>
class [[nodiscard]] coff_debug_directory_base
	: public detail::packed_struct_base<detail::debug::image_coff_symbols_header>
	, public Bases...
{
public:
	using symbol_list_type = std::vector<coff_symbol>;
	using string_table_length_type = packed_struct<std::uint32_t>;

public:
	[[nodiscard]]
	symbol_list_type& get_symbols() & noexcept
	{
		return coff_symbols_;
	}

	[[nodiscard]]
	const symbol_list_type& get_symbols() const& noexcept
	{
		return coff_symbols_;
	}

	[[nodiscard]]
	symbol_list_type get_symbols() && noexcept
	{
		return std::move(coff_symbols_);
	}
	
	[[nodiscard]]
	string_table_length_type& get_string_table_length() noexcept
	{
		return string_table_length_;
	}

	[[nodiscard]]
	const string_table_length_type& get_string_table_length() const noexcept
	{
		return string_table_length_;
	}

	[[nodiscard]]
	buffers::ref_buffer& get_string_table_buffer() & noexcept
	{
		return string_table_;
	}

	[[nodiscard]]
	const buffers::ref_buffer& get_string_table_buffer() const& noexcept
	{
		return string_table_;
	}

	[[nodiscard]]
	buffers::ref_buffer get_string_table_buffer() && noexcept
	{
		return std::move(string_table_);
	}

private:
	symbol_list_type coff_symbols_;
	string_table_length_type string_table_length_;
	buffers::ref_buffer string_table_;
};

using coff_debug_directory = coff_debug_directory_base<>;
using coff_debug_directory_details = coff_debug_directory_base<error_list>;

template<typename Descriptor, typename... Bases>
class [[nodiscard]] codeview_pdb_debug_directory_base
	: public detail::packed_struct_base<Descriptor>
	, public Bases...
{
public:
	[[nodiscard]]
	packed_c_string& get_pdb_file_name() & noexcept
	{
		return pdb_file_name_;
	}

	[[nodiscard]]
	const packed_c_string& get_pdb_file_name() const& noexcept
	{
		return pdb_file_name_;
	}

	[[nodiscard]]
	packed_c_string get_pdb_file_name() && noexcept
	{
		return std::move(pdb_file_name_);
	}

private:
	packed_c_string pdb_file_name_;
};

template<typename... Bases>
using codeview_pdb7_debug_directory_base
	= codeview_pdb_debug_directory_base<detail::debug::cv_info_pdb70, Bases...>;
template<typename... Bases>
using codeview_pdb2_debug_directory_base
	= codeview_pdb_debug_directory_base<detail::debug::cv_info_pdb20, Bases...>;

using codeview_pdb7_debug_directory = codeview_pdb7_debug_directory_base<>;
using codeview_pdb7_debug_directory_details
	= codeview_pdb7_debug_directory_base<error_list>;
using codeview_pdb2_debug_directory = codeview_pdb2_debug_directory_base<>;
using codeview_pdb2_debug_directory_details
	= codeview_pdb2_debug_directory_base<error_list>;

enum class codeview_signature
{
	pdb2 = detail::debug::debug_signature_pdb2,
	pdb7 = detail::debug::debug_signature_pdb7,
	nb02 = detail::debug::debug_signature_nb02,
	nb05 = detail::debug::debug_signature_nb05,
	nb06 = detail::debug::debug_signature_nb06,
	nb07 = detail::debug::debug_signature_nb07,
	nb08 = detail::debug::debug_signature_nb08,
	nb09 = detail::debug::debug_signature_nb09, //codeview 4.1
	nb11 = detail::debug::debug_signature_nb11 //codeview 5.0
};

template<typename... Bases>
class [[nodiscard]] codeview_omf_debug_directory_base : public Bases...
{
public:
	using signature_type = packed_struct<std::uint32_t>;

public:
	[[nodiscard]]
	signature_type& get_signature() noexcept
	{
		return signature_;
	}

	[[nodiscard]]
	const signature_type& get_signature() const noexcept
	{
		return signature_;
	}

	[[nodiscard]]
	codeview_signature get_codeview_signature() noexcept
	{
		return static_cast<codeview_signature>(signature_.get());
	}

private:
	signature_type signature_;
};

using codeview_omf_debug_directory = codeview_omf_debug_directory_base<>;
using codeview_omf_debug_directory_details = codeview_omf_debug_directory_base<error_list>;

enum class fpo_frame_type
{
	fpo = detail::debug::frame_type_fpo,
	trap = detail::debug::frame_type_trap,
	tss = detail::debug::frame_type_tss,
	nonfpo = detail::debug::frame_type_nonfpo
};

class [[nodiscard]] fpo_entry
	: public detail::packed_struct_base<detail::debug::fpo_data>
{
public:
	[[nodiscard]]
	std::uint8_t get_prolog_byte_count() const noexcept
	{
		return static_cast<std::uint8_t>(descriptor_->flags & 0xffu);
	}

	[[nodiscard]]
	std::uint8_t get_saved_regs() const noexcept
	{
		return static_cast<std::uint8_t>(
			(descriptor_->flags & 0x700u) >> 8u);
	}

	[[nodiscard]]
	bool is_seh_in_func() const noexcept
	{
		return static_cast<bool>(descriptor_->flags & 0x800u);
	}

	[[nodiscard]]
	bool is_ebp_allocated() const noexcept
	{
		return static_cast<bool>(descriptor_->flags & 0x1000u);
	}

	[[nodiscard]]
	fpo_frame_type get_frame_type() const noexcept
	{
		return static_cast<fpo_frame_type>(
			(descriptor_->flags & 0xc000u) >> 14u);
	}
};

template<typename... Bases>
class [[nodiscard]] fpo_debug_directory_base : public Bases...
{
public:
	using entry_list_type = std::vector<fpo_entry>;

public:
	[[nodiscard]]
	entry_list_type& get_entries() & noexcept
	{
		return entries_;
	}

	[[nodiscard]]
	const entry_list_type& get_entries() const& noexcept
	{
		return entries_;
	}

	[[nodiscard]]
	entry_list_type get_entries() && noexcept
	{
		return std::move(entries_);
	}

private:
	entry_list_type entries_;
};

using fpo_debug_directory = fpo_debug_directory_base<>;
using fpo_debug_directory_details = fpo_debug_directory_base<error_list>;

enum class misc_data_type
{
	exename = detail::debug::image_debug_misc_exename
};

template<typename... Bases>
class [[nodiscard]] misc_debug_directory_base
	: public detail::packed_struct_base<detail::debug::image_debug_misc>
	, public Bases...
{
public:
	using data_type = std::variant<packed_c_string, packed_utf16_c_string>;

public:
	[[nodiscard]]
	data_type& get_data() & noexcept
	{
		return data_;
	}

	[[nodiscard]]
	const data_type& get_data() const& noexcept
	{
		return data_;
	}

	[[nodiscard]]
	data_type get_data() && noexcept
	{
		return std::move(data_);
	}

	[[nodiscard]]
	misc_data_type get_data_type() const noexcept
	{
		return static_cast<misc_data_type>(descriptor_->data_type);
	}

private:
	data_type data_;
};

using misc_debug_directory = misc_debug_directory_base<>;
using misc_debug_directory_details = misc_debug_directory_base<error_list>;

struct omap_to_src_tag {};
struct src_to_omap_tag {};

template<typename Tag, typename... Bases>
class [[nodiscard]] omap_src_mapping_debug_directory_base : public Bases...
{
public:
	using mapping_type = packed_struct<detail::debug::image_debug_omap>;
	using mapping_list_type = std::vector<mapping_type>;

public:
	[[nodiscard]]
	mapping_list_type& get_mappings() & noexcept
	{
		return mappings_;
	}

	[[nodiscard]]
	const mapping_list_type& get_mappings() const& noexcept
	{
		return mappings_;
	}
	
	[[nodiscard]]
	mapping_list_type get_mappings() && noexcept
	{
		return std::move(mappings_);
	}

private:
	mapping_list_type mappings_;
};

template<typename... Bases>
using omap_to_src_debug_directory_base = omap_src_mapping_debug_directory_base<
	omap_to_src_tag, Bases...>;
template<typename... Bases>
using src_to_omap_debug_directory_base = omap_src_mapping_debug_directory_base<
	src_to_omap_tag, Bases...>;

using omap_to_src_debug_directory = omap_to_src_debug_directory_base<>;
using omap_to_src_debug_directory_details = omap_to_src_debug_directory_base<
	error_list>;
using src_to_omap_debug_directory = src_to_omap_debug_directory_base<>;
using src_to_omap_debug_directory_details = src_to_omap_debug_directory_base<
	error_list>;

template<typename... Bases>
class [[nodiscard]] vc_feature_debug_directory_base
	: public detail::packed_struct_base<detail::debug::image_debug_vc_feature>
	, public Bases...
{
};

using vc_feature_debug_directory = vc_feature_debug_directory_base<>;
using vc_feature_debug_directory_details = vc_feature_debug_directory_base<error_list>;

enum class pogo_type
{
	ltcg = detail::debug::pogo_type::ltcg,
	pgu = detail::debug::pogo_type::pgu,
	pgi = detail::debug::pogo_type::pgi,
	pgo = detail::debug::pogo_type::pgo
};

class [[nodiscard]] pogo_debug_entry
	: public detail::packed_struct_base<detail::debug::pogo_entry>
{
public:
	[[nodiscard]]
	packed_c_string& get_name() & noexcept
	{
		return name_;
	}

	[[nodiscard]]
	const packed_c_string& get_name() const& noexcept
	{
		return name_;
	}

	[[nodiscard]]
	packed_c_string get_name() && noexcept
	{
		return std::move(name_);
	}

private:
	packed_c_string name_;
};

template<typename... Bases>
class [[nodiscard]] pogo_debug_directory_base
	: public detail::packed_struct_base<detail::debug::pogo_header>
	, public Bases...
{
public:
	using entry_list_type = std::vector<pogo_debug_entry>;

public:
	[[nodiscard]]
	pogo_type get_pogo_type() const noexcept
	{
		return static_cast<pogo_type>(descriptor_->signature);
	}

	[[nodiscard]]
	entry_list_type& get_entries() & noexcept
	{
		return entries_;
	}

	[[nodiscard]]
	const entry_list_type& get_entries() const& noexcept
	{
		return entries_;
	}

	[[nodiscard]]
	entry_list_type get_entries() && noexcept
	{
		return std::move(entries_);
	}

private:
	entry_list_type entries_;
};

using pogo_debug_directory = pogo_debug_directory_base<>;
using pogo_debug_directory_details = pogo_debug_directory_base<error_list>;

class [[nodiscard]] iltcg_debug_directory {};

struct mpx_flags final
{
	enum value
	{
		enable = detail::debug::image_mpx_enable,
		enable_driver_runtime = detail::debug::image_mpx_enable_driver_runtime,
		enable_fast_fail_on_bnd_exception
			= detail::debug::image_mpx_enable_fast_fail_on_bnd_exception
	};
};

template<typename... Bases>
class [[nodiscard]] mpx_debug_directory_base
	: public detail::packed_struct_base<detail::debug::image_debug_mpx>
	, public Bases...
{
public:
	static constexpr std::uint32_t signature = detail::debug::mpx_signature;

	[[nodiscard]]
	mpx_flags::value get_flags() const noexcept
	{
		return static_cast<mpx_flags::value>(descriptor_->flags);
	}
};

using mpx_debug_directory = mpx_debug_directory_base<>;
using mpx_debug_directory_details = mpx_debug_directory_base<error_list>;

template<typename... Bases>
class [[nodiscard]] repro_debug_directory_base : public Bases...
{
public:
	using descriptor_type = packed_struct<detail::debug::image_debug_repro>;
	static constexpr std::size_t max_hash_length = 1024;

public:
	[[nodiscard]]
	std::optional<descriptor_type>& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const std::optional<descriptor_type>& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	std::optional<packed_byte_vector>& get_hash() & noexcept
	{
		return hash_;
	}

	[[nodiscard]]
	const std::optional<packed_byte_vector>& get_hash() const& noexcept
	{
		return hash_;
	}

	[[nodiscard]]
	std::optional<packed_byte_vector> get_hash() && noexcept
	{
		return std::move(hash_);
	}

private:
	std::optional<descriptor_type> descriptor_;
	std::optional<packed_byte_vector> hash_;
};

using repro_debug_directory = repro_debug_directory_base<>;
using repro_debug_directory_details = repro_debug_directory_base<error_list>;

template<typename... Bases>
class [[nodiscard]] spgo_debug_directory_base : public Bases...
{
public:
	[[nodiscard]]
	packed_utf16_c_string& get_string() & noexcept
	{
		return string_;
	}

	[[nodiscard]]
	const packed_utf16_c_string& get_string() const& noexcept
	{
		return string_;
	}

	[[nodiscard]]
	packed_utf16_c_string get_string() && noexcept
	{
		return std::move(string_);
	}

private:
	packed_utf16_c_string string_;
};

using spgo_debug_directory = spgo_debug_directory_base<>;
using spgo_debug_directory_details = spgo_debug_directory_base<error_list>;

struct image_dllcharacteristics_ex final
{
	enum value
	{
		cet_compat = detail::debug::image_dllcharacteristics_ex::cet_compat,
		cet_compat_strict_mode
			= detail::debug::image_dllcharacteristics_ex::cet_compat_strict_mode,
		cet_set_context_ip_validation_relaxed_mode
			= detail::debug::image_dllcharacteristics_ex::cet_set_context_ip_validation_relaxed_mode,
		cet_dynamic_apis_allow_in_proc
			= detail::debug::image_dllcharacteristics_ex::cet_dynamic_apis_allow_in_proc,
		cet_reserved_1 = detail::debug::image_dllcharacteristics_ex::cet_reserved_1,
		cet_reserved_2 = detail::debug::image_dllcharacteristics_ex::cet_reserved_2
	};
}; //namespace image_dllcharacteristics_ex

template<typename... Bases>
class [[nodiscard]] ex_dllcharacteristics_debug_directory_base
	: public detail::packed_struct_base<detail::debug::image_debug_dllcharacteristics_ex>
	, public Bases...
{
public:
	[[nodiscard]]
	image_dllcharacteristics_ex::value get_characteristics() const noexcept
	{
		return static_cast<image_dllcharacteristics_ex::value>(descriptor_->flags);
	}
};

using ex_dllcharacteristics_debug_directory
	= ex_dllcharacteristics_debug_directory_base<>;
using ex_dllcharacteristics_debug_directory_details
	= ex_dllcharacteristics_debug_directory_base<error_list>;

enum class debug_directory_type
{
	coff = detail::debug::image_debug_type::coff,
	codeview = detail::debug::image_debug_type::codeview,
	fpo = detail::debug::image_debug_type::fpo,
	misc = detail::debug::image_debug_type::misc,
	exception = detail::debug::image_debug_type::exception,
	fixup = detail::debug::image_debug_type::fixup,
	omap_to_src = detail::debug::image_debug_type::omap_to_src,
	omap_from_src = detail::debug::image_debug_type::omap_from_src,
	borland = detail::debug::image_debug_type::borland,
	bbt = detail::debug::image_debug_type::bbt,
	clsid = detail::debug::image_debug_type::clsid,
	vc_feature = detail::debug::image_debug_type::vc_feature,
	pogo = detail::debug::image_debug_type::pogo,
	iltcg = detail::debug::image_debug_type::iltcg,
	mpx = detail::debug::image_debug_type::mpx,
	repro = detail::debug::image_debug_type::repro,
	spgo = detail::debug::image_debug_type::spgo,
	ex_dllcharacteristics = detail::debug::image_debug_type::ex_dllcharacteristics
};

struct [[nodiscard]] debug_directory_parse_options
{
	bool allow_virtual_data = false;
	std::uint32_t max_coff_symbol_count = 100000u;
	std::uint32_t max_debug_entry_count = 10000u;
	//Optional, used for COFF validation purposes
	const core::file_header* file_header{};
	bool copy_coff_string_table_memory = false;
};

template<typename... Bases>
class [[nodiscard]] debug_directory_base
	: public detail::packed_struct_base<detail::debug::image_debug_directory>
	, public Bases...
{
public:
	using underlying_directory_type = std::variant<
		coff_debug_directory_details,
		codeview_pdb7_debug_directory_details,
		codeview_pdb2_debug_directory_details,
		codeview_omf_debug_directory_details,
		fpo_debug_directory_details,
		misc_debug_directory_details,
		omap_to_src_debug_directory_details,
		src_to_omap_debug_directory_details,
		vc_feature_debug_directory_details,
		pogo_debug_directory_details,
		iltcg_debug_directory,
		mpx_debug_directory_details,
		repro_debug_directory_details,
		spgo_debug_directory_details,
		ex_dllcharacteristics_debug_directory_details
	>;

public:
	[[nodiscard]]
	buffers::ref_buffer get_raw_data() && noexcept
	{
		return std::move(raw_data_);
	}

	[[nodiscard]]
	buffers::ref_buffer& get_raw_data() & noexcept
	{
		return raw_data_;
	}

	[[nodiscard]]
	const buffers::ref_buffer& get_raw_data() const& noexcept
	{
		return raw_data_;
	}

	[[nodiscard]]
	debug_directory_type get_type() const noexcept
	{
		return static_cast<debug_directory_type>(descriptor_->type);
	}

	[[nodiscard]]
	underlying_directory_type get_underlying_directory(
		const debug_directory_parse_options& options = {}) const;

private:
	buffers::ref_buffer raw_data_;
};

template<typename... Bases>
class [[nodiscard]] debug_directory_list_base : public Bases...
{
public:
	using list_type = std::vector<debug_directory_base<Bases...>>;

public:
	[[nodiscard]]
	list_type get_entries() && noexcept
	{
		return std::move(list_);
	}

	[[nodiscard]]
	list_type& get_entries() & noexcept
	{
		return list_;
	}

	[[nodiscard]]
	const list_type& get_entries() const& noexcept
	{
		return list_;
	}

private:
	list_type list_;
};

using debug_directory_list = debug_directory_list_base<>;
using debug_directory_list_details = debug_directory_list_base<error_list>;
using debug_directory = debug_directory_base<>;
using debug_directory_details = debug_directory_base<error_list>;

[[nodiscard]]
misc_debug_directory_details parse_misc_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data);

[[nodiscard]]
repro_debug_directory_details parse_repro_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data);

[[nodiscard]]
spgo_debug_directory_details parse_spgo_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data);

[[nodiscard]]
debug_directory_details::underlying_directory_type
	parse_codeview_directory(buffers::input_buffer_stateful_wrapper& wrapper,
		bool allow_virtual_data);

[[nodiscard]]
omap_to_src_debug_directory_details parse_omap_to_src_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options);

[[nodiscard]]
src_to_omap_debug_directory_details parse_src_to_omap_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options);

[[nodiscard]]
fpo_debug_directory_details parse_fpo_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options);

[[nodiscard]]
pogo_debug_directory_details parse_pogo_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options);

[[nodiscard]]
coff_debug_directory_details parse_coff_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	const debug_directory_parse_options& options);

[[nodiscard]]
vc_feature_debug_directory_details parse_vc_feature_debug_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data);

[[nodiscard]]
mpx_debug_directory_details parse_mpx_debug_directory(
	buffers::input_buffer_stateful_wrapper& wrapper,
	bool allow_virtual_data);

[[nodiscard]]
ex_dllcharacteristics_debug_directory_details
	parse_ex_dllcharacteristics_debug_directory(
		buffers::input_buffer_stateful_wrapper& wrapper,
		bool allow_virtual_data);
} //namespace pe_bliss::debug

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::debug::debug_directory_errc> : true_type {};
} //namespace std
