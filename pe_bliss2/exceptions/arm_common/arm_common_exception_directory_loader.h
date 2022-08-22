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
#include "pe_bliss2/exceptions/exception_directory.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace pe_bliss::exceptions::arm_common
{

enum class exception_directory_loader_errc
{
	unaligned_unwind_info = 1,
	unordered_epilog_scopes,
	invalid_unwind_info,
	invalid_runtime_function_entry,
	excessive_data_in_directory
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
	utilities::safe_uint<rva_type> current_rva, RuntimeFunction& func,
	ExtendedUnwindRecord& unwind_info) try
{
	if (!utilities::math::is_aligned<rva_type>(current_rva.value()))
		func.add_error(exception_directory_loader_errc::unaligned_unwind_info);

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
		for (std::uint32_t i = 0, count = unwind_info.get_epilog_count(); i != count; ++i)
		{
			current_rva += struct_from_rva(instance, current_rva.value(),
				epilogs.emplace_back().get_descriptor(), options.include_headers,
				options.allow_virtual_data).packed_size;

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
	auto last_rva = current_rva + byte_count;
	while (current_rva < last_rva)
	{
		packed_struct<std::byte> first_byte;
		current_rva += struct_from_rva(instance, current_rva.value(),
			first_byte, options.include_headers, options.allow_virtual_data).packed_size;

		if (!std::to_integer<std::uint8_t>(first_byte.get()))
		{
			current_rva = last_rva;
			break;
		}

		UwopControl::create_uwop_code(unwind_codes, UwopControl::decode_unwind_code(first_byte.get()));
		std::visit([&first_byte, &current_rva, &instance, &options, &last_rva, &func] (auto& code) {
			auto& descriptor = code.get_descriptor();
			descriptor.copy_metadata_from(first_byte);
			descriptor[0] = first_byte.get();

			if constexpr (code.length > 1u)
			{
				if (current_rva + (code.length - 1u) > last_rva)
					throw pe_error(exception_directory_loader_errc::invalid_unwind_info);

				//Set allow_virtual_data=true, as the check is done on the next line
				auto bytes_read = section_data_from_rva(instance, current_rva.value(),
					options.include_headers, true)->read(0u, code.length - 1u, &descriptor.value()[1]);
				//TODO: add error to uwop code and not the func
				if (bytes_read != code.length - 1u && !options.allow_virtual_data)
					func.add_error(exception_directory_loader_errc::invalid_runtime_function_entry);
				descriptor.set_physical_size(descriptor.physical_size() + bytes_read);
				current_rva += code.length - 1u;
			}
		}, unwind_codes.back());
	}

	if (unwind_info.has_exception_data())
	{
		struct_from_rva(instance, current_rva.value(),
			unwind_info.get_exception_handler_rva(), options.include_headers,
			options.allow_virtual_data);
	}
}
catch (const std::system_error& e)
{
	func.add_error(e.code());
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
	typename ExtendedUnwindRecord, typename ExceptionDirectory, typename LoaderOptions>
void load(const image::image& instance, const LoaderOptions& options,
	pe_bliss::exceptions::exception_directory_details& directory)
{
	auto [current_rva, size] = ExceptionDirectoryControl::get_exception_directory(instance, options);
	if (!current_rva)
		return;

	auto& dir = std::get<ExceptionDirectory>(
		directory.get_directories().emplace_back(std::in_place_type<ExceptionDirectory>));

	utilities::safe_uint last_rva = current_rva;
	last_rva += size;

	auto& runtime_functions = dir.get_runtime_function_list();

	while (current_rva < last_rva.value())
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
		current_rva += static_cast<rva_type>(func.get_descriptor().packed_size);
	}

	if (current_rva != last_rva.value())
		dir.add_error(exception_directory_loader_errc::excessive_data_in_directory);
}

} //namespace namespace pe_bliss::exceptions::arm_common
