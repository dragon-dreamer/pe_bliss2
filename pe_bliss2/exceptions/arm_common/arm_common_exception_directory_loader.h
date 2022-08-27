#pragma once

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <type_traits>
#include <variant>

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/generic_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace pe_bliss::exceptions::arm_common
{

enum class exception_directory_loader_errc
{
	unordered_epilog_scopes = 1,
	invalid_extended_unwind_info,
	invalid_runtime_function_entry,
	invalid_uwop_code,
	invalid_directory_size,
	invalid_exception_handler,
	invalid_exception_handler_rva,
	unmatched_directory_size
};

} //namespace pe_bliss::exceptions::arm_common

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::exceptions::arm_common::exception_directory_loader_errc> : true_type {};
} //namespace std

namespace pe_bliss::exceptions::arm_common
{

std::error_code make_error_code(exception_directory_loader_errc) noexcept;

struct [[nodiscard]] exception_directory_info
{
	rva_type rva{};
	rva_type size{};
};

template<typename UwopControl, typename LoaderOptions,
	typename RuntimeFunction, typename ExtendedUnwindRecord>
void load_extended_unwind_record(
	const image::image& instance, const LoaderOptions& options,
	utilities::safe_uint<rva_type> current_rva,
	RuntimeFunction& func, ExtendedUnwindRecord& unwind_info) try
{
	//No need to check if aligned or not. Always aligned, because
	//two lower bits of RVA is zero (func.has_extended_unwind_record() == true).
	current_rva += struct_from_rva(instance, current_rva.value(),
		unwind_info.get_main_header(), options.include_headers,
		options.allow_virtual_data).packed_size;

	if (unwind_info.has_extended_main_header())
	{
		current_rva += struct_from_rva(instance, current_rva.value(),
			unwind_info.get_main_extended_header(), options.include_headers,
			options.allow_virtual_data).packed_size;
	}

	if (!unwind_info.single_epilog_info_packed())
	{
		auto& epilogs = unwind_info.get_epilog_info_list();
		std::uint32_t prev_start_offset{};
		bool ordered = true;
		auto count = unwind_info.get_epilog_count();
		epilogs.reserve(count);
		while (count--)
		{
			try
			{
				current_rva += struct_from_rva(instance, current_rva.value(),
					epilogs.emplace_back().get_descriptor(), options.include_headers,
					options.allow_virtual_data).packed_size;
			}
			catch (const std::system_error&)
			{
				epilogs.pop_back();
				throw;
			}

			auto start_offset = epilogs.back().get_epilog_start_offset();
			if (start_offset < prev_start_offset)
				ordered = false;
			else
				prev_start_offset = start_offset;
		}

		if (!ordered)
			func.add_error(exception_directory_loader_errc::unordered_epilog_scopes);
	}

	auto& unwind_codes = unwind_info.get_unwind_code_list();
	auto byte_count = unwind_info.get_code_words() * sizeof(std::uint32_t);
	auto last_opcode_rva = current_rva + byte_count;
	std::size_t opcode_index = 0;
	while (current_rva < last_opcode_rva)
	{
		packed_struct<std::byte> first_byte;
		current_rva += struct_from_rva(instance, current_rva.value(),
			first_byte, options.include_headers, options.allow_virtual_data).packed_size;

		if (!std::to_integer<std::uint8_t>(first_byte.get()))
		{
			current_rva = last_opcode_rva;
			break;
		}

		UwopControl::create_uwop_code(unwind_codes,
			UwopControl::decode_unwind_code(first_byte.get()));

		std::visit([&first_byte, &current_rva, &instance,
			&options, &last_opcode_rva, &func, opcode_index] (auto& code) {
			auto& descriptor = code.get_descriptor();
			descriptor.copy_metadata_from(first_byte);
			descriptor[0] = first_byte.get();

			if constexpr (code.length > 1u)
			{
				if (current_rva + (code.length - 1u) > last_opcode_rva)
					throw pe_error(utilities::generic_errc::buffer_overrun);

				std::size_t bytes_read{};
				try
				{
					//Set allow_virtual_data=true, as the check is done on the next line
					bytes_read = section_data_from_rva(instance, current_rva.value(),
						options.include_headers, true)
						->read(0u, code.length - 1u, &descriptor.value()[1]);
					if (bytes_read != code.length - 1u && !options.allow_virtual_data)
						throw pe_error(utilities::generic_errc::buffer_overrun);
				}
				catch (const std::system_error&)
				{
					func.add_error(exception_directory_loader_errc::invalid_uwop_code, opcode_index);
					throw;
				}
				descriptor.set_physical_size(descriptor.physical_size() + bytes_read);
				descriptor.set_data_size(code.length);
				current_rva += code.length - 1u;
			}
		}, unwind_codes.back());
		++opcode_index;
	}

	if (unwind_info.has_exception_data())
	{
		try
		{
			struct_from_rva(instance, current_rva.value(),
				unwind_info.get_exception_handler_rva(), options.include_headers,
				options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			func.add_error(exception_directory_loader_errc::invalid_exception_handler);
			return;
		}

		try
		{
			[[maybe_unused]] auto first_byte = struct_from_rva<std::uint8_t>(
				instance, unwind_info.get_exception_handler_rva().get(),
				options.include_headers, options.allow_virtual_data);
		}
		catch (const std::system_error&)
		{
			func.add_error(exception_directory_loader_errc::invalid_exception_handler_rva);
			return;
		}
	}
}
catch (const std::system_error&)
{
	func.add_error(exception_directory_loader_errc::invalid_extended_unwind_info);
}

template<typename UwopControl, typename PackedUnwindData, typename ExtendedUnwindRecord,
	typename LoaderOptions, typename RuntimeFunction>
void load_runtime_function(const image::image& instance, const LoaderOptions& options,
	rva_type current_rva, RuntimeFunction& func)
{
	struct_from_rva(instance, current_rva, func.get_descriptor(),
		options.include_headers, options.allow_virtual_data);

	if (func.has_extended_unwind_record())
	{
		load_extended_unwind_record<UwopControl>(
			instance, options, func.get_descriptor()->unwind_data,
			func, func.get_unwind_info().emplace<ExtendedUnwindRecord>());
	}
	else
	{
		func.get_unwind_info().emplace<PackedUnwindData>(
			func.get_descriptor()->unwind_data);
	}
}

template<typename ExceptionDirectoryControl, typename UwopControl, typename PackedUnwindData,
	typename ExtendedUnwindRecord, typename ExceptionDirectory, typename LoaderOptions,
	typename DirectoryContainer>
void load(const image::image& instance, const LoaderOptions& options,
	DirectoryContainer& directory_container)
{
	auto [current_rva, size] = ExceptionDirectoryControl
		::get_exception_directory(instance, options);
	if (!current_rva)
		return;

	auto& dir = std::get<ExceptionDirectory>(
		directory_container.get_directories().emplace_back(
			std::in_place_type<ExceptionDirectory>));

	utilities::safe_uint last_rva = current_rva;
	try
	{
		last_rva += size;
	}
	catch (const std::system_error&)
	{
		dir.add_error(exception_directory_loader_errc::invalid_directory_size);
		return;
	}

	auto& runtime_functions = dir.get_runtime_function_list();
	static constexpr std::uint32_t runtime_function_descriptor_size =
		std::remove_cvref_t<decltype(runtime_functions.back().get_descriptor())>::packed_size;
	while (current_rva + runtime_function_descriptor_size <= last_rva)
	{
		auto& func = runtime_functions.emplace_back();
		try
		{
			load_runtime_function<UwopControl, PackedUnwindData, ExtendedUnwindRecord>(
				instance, options, current_rva, func);
		}
		catch (const std::system_error&)
		{
			func.add_error(exception_directory_loader_errc::invalid_runtime_function_entry);
		}
		//Does not overflow, checked above
		current_rva += runtime_function_descriptor_size;
	}
	if (current_rva != last_rva)
		dir.add_error(exception_directory_loader_errc::unmatched_directory_size);
}

} //namespace namespace pe_bliss::exceptions::arm_common
