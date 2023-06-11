#include "pe_bliss2/exceptions/x64/x64_exception_directory_loader.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <system_error>
#include <utility>

#include "buffers/input_buffer_stateful_wrapper.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/core/file_header.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/exceptions/x64/x64_exception_directory.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{

struct x64_exception_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "x64_exception_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::exceptions::x64::exception_directory_loader_errc;
		switch (static_cast<pe_bliss::exceptions::x64::exception_directory_loader_errc>(ev))
		{
		case unaligned_runtime_function_entry:
			return "Unaligned runtime function entry";
		case invalid_unwind_info_flags:
			return "Invalid unwind info flags";
		case unmatched_directory_size:
			return "Unmatched directory size";
		case unaligned_unwind_info:
			return "Unaligned unwind information";
		case invalid_unwind_slot_count:
			return "Invalid unwind slot count";
		case unknown_unwind_code:
			return "Unknown unwind code";
		case push_nonvol_uwop_out_of_order:
			return "PUSH_NONVOL unwind code is out of order with other codes";
		case invalid_runtime_function_entry:
			return "Invalid runtime function entry";
		case invalid_unwind_info:
			return "Invalid unwind info";
		case invalid_exception_handler_rva:
			return "Invalid exception handler RVA";
		case invalid_chained_runtime_function_entry:
			return "Invalid chained runtime function entry";
		case both_set_fpreg_types_used:
			return "Both SET_FPREG and SET_FPREG_LARGE are used";
		case invalid_directory_size:
			return "Invalid exception directory size";
		case invalid_c_specific_handler_record_count:
			return "Invalid C-specific handler record count";
		case too_many_c_specific_handler_records:
			return "Too many C-specific handler records";
		case invalid_c_specific_handler_record:
			return "Invalid C-specific handler record";
		default:
			return {};
		}
	}
};

const x64_exception_directory_loader_error_category
	x64_exception_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::exceptions::x64;

template<typename Code, typename Buffer>
void load_uwop_code(bool allow_virtual_data, const opcode_base<>& base_descriptor,
	Code& code, Buffer& unwind_codes_data)
{
	auto& new_descriptor = code.get_descriptor();
	new_descriptor.copy_metadata_from(base_descriptor.get_descriptor());
	new_descriptor->offset_in_prolog
		= base_descriptor.get_descriptor()->offset_in_prolog;
	new_descriptor->unwind_operation_code_and_info
		= base_descriptor.get_descriptor()->unwind_operation_code_and_info;

	if constexpr (Code::node_count != 0u)
	{
		packed_struct<decltype(Code::descriptor_type::value_type::node)> node;
		node.deserialize(unwind_codes_data, allow_virtual_data);
		new_descriptor->node = node.get();
		new_descriptor.set_physical_size(new_descriptor.physical_size() + node.physical_size());
	}
}

template<typename RuntimeFunction, typename Buffer>
void load_unwind_codes(RuntimeFunction& func, Buffer& unwind_codes_data, bool allow_virtual_data)
{
	auto& unwind_info = func.get_unwind_info();
	utilities::safe_uint unwind_code_count = unwind_info.get_descriptor()->count_of_unwind_codes;
	auto& unwind_code_list = unwind_info.get_unwind_code_list();
	bool can_be_push_nonvol_or_machframe_only = false;
	bool set_fpreg_large_used = false, set_fpreg_used = false;
	while (unwind_code_count)
	{
		opcode_base<> base_code_descriptor;
		base_code_descriptor.get_descriptor().deserialize(unwind_codes_data, allow_virtual_data);
		auto uwop_code = base_code_descriptor.get_uwop_code();
		switch (uwop_code)
		{
		case opcode_id::push_nonvol:
			can_be_push_nonvol_or_machframe_only = true;
			unwind_code_list.emplace_back(std::in_place_type<push_nonvol>);
			break;
		case opcode_id::alloc_large:
			if (!base_code_descriptor.get_operation_info())
				unwind_code_list.emplace_back(std::in_place_type<alloc_large<1u>>);
			else
				unwind_code_list.emplace_back(std::in_place_type<alloc_large<2u>>);
			break;
		case opcode_id::alloc_small:
			unwind_code_list.emplace_back(std::in_place_type<alloc_small>);
			break;
		case opcode_id::set_fpreg:
			unwind_code_list.emplace_back(std::in_place_type<set_fpreg>);
			set_fpreg_used = true;
			break;
		case opcode_id::save_nonvol:
			unwind_code_list.emplace_back(std::in_place_type<save_nonvol>);
			break;
		case opcode_id::save_nonvol_far:
			unwind_code_list.emplace_back(std::in_place_type<save_nonvol_far>);
			break;
		case opcode_id::save_xmm128:
			unwind_code_list.emplace_back(std::in_place_type<save_xmm128>);
			break;
		case opcode_id::save_xmm128_far:
			unwind_code_list.emplace_back(std::in_place_type<save_xmm128_far>);
			break;
		case opcode_id::push_machframe:
			unwind_code_list.emplace_back(std::in_place_type<push_machframe>);
			break;
		case opcode_id::set_fpreg_large:
			unwind_code_list.emplace_back(std::in_place_type<set_fpreg_large>);
			set_fpreg_large_used = true;
			break;
		case opcode_id::spare:
			if (unwind_info.get_version() >= 2u)
			{
				unwind_code_list.emplace_back(std::in_place_type<spare>);
				break;
			}
			[[fallthrough]];
		case opcode_id::epilog:
			if (unwind_info.get_version() >= 2u)
			{
				unwind_code_list.emplace_back(std::in_place_type<epilog>);
				break;
			}
			[[fallthrough]];
		default:
			func.add_error(exception_directory_loader_errc::unknown_unwind_code);
			return;
		}

		if (can_be_push_nonvol_or_machframe_only
			&& uwop_code != opcode_id::push_nonvol && uwop_code != opcode_id::push_machframe)
		{
			func.add_error(exception_directory_loader_errc::push_nonvol_uwop_out_of_order);
		}

		std::visit([&unwind_code_count, &base_code_descriptor,
			&func, &unwind_codes_data, allow_virtual_data] (auto& code) {
			if (unwind_code_count < code.node_count + 1u)
			{
				unwind_code_count = 0u;
				func.add_error(exception_directory_loader_errc::invalid_unwind_slot_count);
				return;
			}

			unwind_code_count -= code.node_count + 1u;
			load_uwop_code(allow_virtual_data,
				base_code_descriptor, code, unwind_codes_data);
		}, unwind_code_list.back());
	}

	if (set_fpreg_large_used && set_fpreg_used)
		func.add_error(exception_directory_loader_errc::both_set_fpreg_types_used);

	if (unwind_code_count)
		func.add_error(exception_directory_loader_errc::invalid_unwind_slot_count);
}

bool load_runtime_function(const image::image& instance, const loader_options& options,
	rva_type current_rva, runtime_function_details& func)
{
	if (!utilities::math::is_aligned<rva_type>(current_rva))
		func.add_error(exception_directory_loader_errc::unaligned_runtime_function_entry);

	struct_from_rva(instance, current_rva, func.get_descriptor(),
		options.include_headers, options.allow_virtual_data);

	const auto& underlying_struct = func.get_descriptor().get();
	if (!underlying_struct.begin_address && !underlying_struct.end_address
		&& !underlying_struct.unwind_info_address)
	{
		return false;
	}

	utilities::safe_uint unwind_info_rva = underlying_struct.unwind_info_address;
	if (!utilities::math::is_aligned<rva_type>(unwind_info_rva.value()))
		func.add_error(exception_directory_loader_errc::unaligned_unwind_info);

	auto& unwind_info = func.get_unwind_info();
	try
	{
		unwind_info_rva += struct_from_rva(instance, unwind_info_rva.value(),
			unwind_info.get_descriptor(), options.include_headers,
			options.allow_virtual_data).packed_size;
	}
	catch (const std::system_error&)
	{
		func.add_error(exception_directory_loader_errc::invalid_unwind_info);
		return true;
	}

	std::uint32_t unwind_code_size = unwind_info.get_descriptor()->count_of_unwind_codes;
	if (unwind_code_size)
	{
		try
		{
			auto unwind_codes_data = section_data_from_rva(instance, unwind_info_rva.value(),
				unwind_code_size * sizeof(std::uint16_t),
				options.include_headers, options.allow_virtual_data);
			buffers::input_buffer_stateful_wrapper_ref wrapper(*unwind_codes_data);
			load_unwind_codes(func, wrapper, options.allow_virtual_data);
			unwind_info_rva += utilities::math::align_up(unwind_code_size, 2u)
				* sizeof(std::uint16_t);
		}
		catch (const std::system_error&)
		{
			func.add_error(exception_directory_loader_errc::invalid_unwind_info);
			return true;
		}
	}

	auto flags = unwind_info.get_unwind_flags();
	if ((flags & unwind_flags::chaininfo)
		&& (flags & (unwind_flags::ehandler | unwind_flags::uhandler)))
	{
		func.add_error(exception_directory_loader_errc::invalid_unwind_info_flags);
		return true;
	}

	if (flags & unwind_flags::chaininfo)
	{
		auto chained_func = std::make_unique<runtime_function_details>();
		try
		{
			if (!load_runtime_function(instance, options,
				unwind_info_rva.value(), *chained_func))
			{
				func.add_error(
					exception_directory_loader_errc::invalid_chained_runtime_function_entry);
			}
		}
		catch (const std::system_error&)
		{
			func.add_error(
				exception_directory_loader_errc::invalid_chained_runtime_function_entry);
		}
		func.get_additional_info() = std::move(chained_func);
	}
	else if (flags & (unwind_flags::ehandler | unwind_flags::uhandler))
	{
		auto& rva = func.get_additional_info()
			.emplace<runtime_function_details::exception_handler_rva_type>();

		try
		{
			struct_from_rva(instance, unwind_info_rva.value(), rva,
				options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			func.add_error(exception_directory_loader_errc::invalid_exception_handler_rva);
			func.get_additional_info().emplace<std::monostate>();
			return true;
		}

		try
		{
			[[maybe_unused]] auto first_byte = struct_from_rva<std::uint8_t>(
				instance, rva.get(), options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			func.add_error(exception_directory_loader_errc::invalid_exception_handler_rva);
		}

		if (flags == unwind_flags::ehandler && options.load_c_specific_handlers)
		{
			auto& scope_table = func.get_scope_table().emplace();

			try
			{
				unwind_info_rva += std::remove_cvref_t<decltype(rva)>::packed_size;
				struct_from_rva(instance, unwind_info_rva.value(),
					scope_table.get_scope_record_count(),
					options.include_headers, options.allow_virtual_data);
				unwind_info_rva += scope_table::count_type::packed_size;
			}
			catch (const std::system_error&)
			{
				func.add_error(
					exception_directory_loader_errc::invalid_c_specific_handler_record_count);
				func.get_scope_table().reset();
				return true;
			}

			auto record_count = (std::min)(scope_table.get_scope_record_count().get(),
				options.max_c_specific_records);
			if (record_count > options.max_c_specific_records)
			{
				func.add_error(
					exception_directory_loader_errc::too_many_c_specific_handler_records);
			}

			while (record_count--)
			{
				auto& record = scope_table.get_records().emplace_back();
				try
				{
					struct_from_rva(instance, unwind_info_rva.value(),
						record, options.include_headers, options.allow_virtual_data);
					unwind_info_rva += scope_table::record_type::packed_size;
				}
				catch (const std::system_error&)
				{
					func.add_error(
						exception_directory_loader_errc::invalid_c_specific_handler_record);
					scope_table.get_records().pop_back();
					return true;
				}
			}
		}
	}

	return true;
}

} //namespace

namespace pe_bliss::exceptions::x64
{

std::error_code make_error_code(exception_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), x64_exception_directory_loader_error_category_instance };
}

void load(const image::image& instance, const loader_options& options,
	pe_bliss::exceptions::exception_directory_details& directory)
{
	if (!instance.is_64bit()
		|| instance.get_file_header().get_machine_type()
			!= core::file_header::machine_type::amd64
		|| !instance.get_data_directories().has_exception_directory())
	{
		return;
	}

	auto data_dir = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::exception);

	auto& x64_dir = std::get<exception_directory_details>(
		directory.get_directories().emplace_back(std::in_place_type<exception_directory_details>));

	auto current_rva = data_dir->virtual_address;
	utilities::safe_uint last_rva = current_rva;
	rva_type last_valid_rva{};
	try
	{
		last_rva += data_dir->size;
		last_valid_rva = (last_rva
			- runtime_function_details::descriptor_type::packed_size).value();
	}
	catch (const std::system_error&)
	{
		x64_dir.add_error(exception_directory_loader_errc::invalid_directory_size);
		return;
	}
	
	auto& runtime_functions = x64_dir.get_runtime_function_list();
	while (current_rva <= last_valid_rva)
	{
		runtime_function_details& func = runtime_functions.emplace_back();
		try
		{
			if (!load_runtime_function(instance, options, current_rva, func))
				runtime_functions.pop_back();
		}
		catch (const std::system_error&)
		{
			func.add_error(exception_directory_loader_errc::invalid_runtime_function_entry);
			break;
		}
		//Does not overflow, checked above
		current_rva += runtime_function_details::descriptor_type::packed_size;
	}

	if (current_rva != last_rva)
		x64_dir.add_error(exception_directory_loader_errc::unmatched_directory_size);
}

} //namespace pe_bliss::exceptions::x64
