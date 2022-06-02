#include "pe_bliss2/tls/tls_directory_builder.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <system_error>
#include <variant>

#include "buffers/output_buffer_interface.h"
#include "buffers/output_memory_ref_buffer.h"
#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/section_data_from_va.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/generic_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{

struct tls_directory_builder_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "tls_directory_builder";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::tls::tls_directory_builder_errc;
		switch (static_cast<pe_bliss::tls::tls_directory_builder_errc>(ev))
		{
		case invalid_raw_data_size:
			return "End address of raw data must be after start address of raw data";
		default:
			return {};
		}
	}
};

const tls_directory_builder_error_category tls_directory_builder_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::tls;

void update_data_directory(image::image& instance, const builder_options& options, std::uint32_t size)
{
	if (options.update_data_directory)
	{
		auto& dir = instance.get_data_directories().get_directory(
			core::data_directories::directory_type::tls);
		dir->virtual_address = options.directory_rva;
		dir->size = size;
	}
}

template<typename Directory>
void build_in_place_impl(image::image& instance, const Directory& directory,
	const builder_options& options)
{
	const auto& descriptor = directory.get_descriptor();
	instance.struct_to_file_offset(descriptor, true, options.write_virtual_part);

	const auto& callbacks = directory.get_callbacks();
	rva_type last_callback_rva{};
	for (const auto& callback : callbacks)
	{
		last_callback_rva = (std::max)(last_callback_rva,
			instance.struct_to_file_offset(callback, true, options.write_virtual_part));
	}

	if (descriptor->address_of_callbacks)
	{
		//Terminator
		instance.struct_to_rva(last_callback_rva, typename Directory::va_type{}, true, true);
	}

	const auto& raw_data = directory.get_raw_data();
	if (descriptor->start_address_of_raw_data && !raw_data.empty())
		instance.buffer_to_file_offset(raw_data, true);

	update_data_directory(instance, options, descriptor.packed_size);
}

template<typename... Directories>
void build_in_place_impl(image::image& instance, const std::variant<Directories...>& directories,
	const builder_options& options)
{
	std::visit([&options, &instance] (const auto& descriptor) {
		build_in_place_impl(instance, descriptor, options);
	}, directories);
}

template<typename Directory>
build_result get_built_size_impl(const Directory& directory, const builder_options& options)
{
	const auto& descriptor = directory.get_descriptor();
	if (descriptor->start_address_of_raw_data > descriptor->end_address_of_raw_data)
		throw pe_error(tls_directory_builder_errc::invalid_raw_data_size);

	if (descriptor->end_address_of_raw_data - descriptor->start_address_of_raw_data >
		(std::numeric_limits<rva_type>::max)())
	{
		throw pe_error(tls_directory_builder_errc::invalid_raw_data_size);
	}

	utilities::safe_uint<std::uint32_t> full_size = descriptor.packed_size;

	using va_type = typename Directory::va_type;
	const auto& callbacks = directory.get_callbacks();
	if (!callbacks.empty())
		full_size += callbacks.size() * sizeof(va_type);

	if (options.align_callbacks)
	{
		auto callbacks_rva = (full_size + options.directory_rva);
		auto aligned_rva = callbacks_rva;
		aligned_rva.align_up(sizeof(va_type));
		full_size += aligned_rva - callbacks_rva;
	}

	full_size += sizeof(va_type); //Callback terminator

	if (options.write_virtual_part)
		full_size += descriptor->end_address_of_raw_data - descriptor->start_address_of_raw_data;
	else
		full_size += directory.get_raw_data().size();

	return {
		.full_size = full_size.value(),
		.directory_size = descriptor.packed_size
	};
}

template<typename... Directories>
build_result get_built_size_impl(const std::variant<Directories...>& directories,
	const builder_options& options)
{
	return std::visit([&options] (const auto& dir) {
		return get_built_size_impl(dir, options);
	}, directories);
}

template<typename Directory>
build_result build_new_impl(buffers::output_buffer_interface& buf, Directory& directory,
	const builder_options& options, std::uint64_t image_base)
{
	auto& descriptor = directory.get_descriptor();
	auto size = get_built_size_impl(directory, options);

	auto base_pos = buf.wpos();

	auto current_pos = base_pos + size.directory_size;
	buf.set_wpos(current_pos);

	utilities::safe_uint<rva_type> current_rva = options.directory_rva;
	current_rva += size.directory_size;
	rva_type alignment_gap{};

	using va_type = typename Directory::va_type;
	if (options.align_callbacks)
	{
		auto aligned_rva = current_rva;
		aligned_rva.align_up(sizeof(va_type));
		alignment_gap = (aligned_rva - current_rva).value();
		if (alignment_gap)
			buf.advance_wpos(alignment_gap);
		current_rva = aligned_rva;
	}

	address_converter conv(image_base);
	descriptor->address_of_callbacks = conv.rva_to_va<va_type>(current_rva.value());

	for (const auto& callback : directory.get_callbacks())
		callback.serialize(buf, true);

	typename Directory::callback_type cb_terminator;
	cb_terminator.serialize(buf, true);

	current_rva += (directory.get_callbacks().size() + 1u) * sizeof(va_type);
	va_type data_length = descriptor->end_address_of_raw_data - descriptor->start_address_of_raw_data;
	descriptor->start_address_of_raw_data = conv.rva_to_va<va_type>(current_rva.value());
	descriptor->end_address_of_raw_data = static_cast<va_type>(
		descriptor->start_address_of_raw_data + data_length);
	current_rva += data_length; //Checks if overflow occurred

	const auto& raw_data = directory.get_raw_data();
	auto raw_data_size = raw_data.size();
	raw_data.serialize(buf, 0, (std::min)(static_cast<std::size_t>(data_length), raw_data_size));
	if (options.write_virtual_part)
	{
		std::array<std::byte, 1> empty{};
		while (data_length > raw_data_size)
		{
			++raw_data_size;
			buf.write(empty.size(), empty.data());
		}
	}

	auto last_pos = buf.wpos();
	buf.set_wpos(base_pos);
	descriptor.serialize(buf, true);

	return {
		.full_size = static_cast<std::uint32_t>(last_pos - base_pos),
		.directory_size = descriptor.packed_size
	};
}

template<typename... Directories>
build_result build_new_impl(buffers::output_buffer_interface& buf,
	std::variant<Directories...>& directories,
	const builder_options& options, std::uint64_t image_base)
{
	assert(options.directory_rva);

	return std::visit([&options, &buf, image_base] (auto& dir) {
		return build_new_impl(buf, dir, options, image_base);
	}, directories);
}

template<typename... Directories>
build_result build_new_impl(image::image& instance, std::variant<Directories...>& directories,
	const builder_options& options, std::uint64_t image_base)
{
	auto buf = buffers::output_memory_ref_buffer(section_data_from_rva(instance,
		options.directory_rva, true));
	auto result = build_new_impl(buf, directories, options, image_base);
	update_data_directory(instance, options, result.directory_size);
	return result;
}

} //namespace

namespace pe_bliss::tls
{

std::error_code make_error_code(tls_directory_builder_errc e) noexcept
{
	return { static_cast<int>(e), tls_directory_builder_error_category_instance };
}

void build_in_place(image::image& instance, const tls_directory_details& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

void build_in_place(image::image& instance, const tls_directory& directory,
	const builder_options& options)
{
	build_in_place_impl(instance, directory, options);
}

build_result build_new(image::image& instance, tls_directory_details& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options,
		instance.get_optional_header().get_raw_image_base());
}

build_result build_new(image::image& instance, tls_directory& directory,
	const builder_options& options)
{
	return build_new_impl(instance, directory, options,
		instance.get_optional_header().get_raw_image_base());
}

build_result build_new(buffers::output_buffer_interface& buf, tls_directory_details& directory,
	const builder_options& options, std::uint64_t image_base)
{
	return build_new_impl(buf, directory, options, image_base);
}

build_result build_new(buffers::output_buffer_interface& buf, tls_directory& directory,
	const builder_options& options, std::uint64_t image_base)
{
	return build_new_impl(buf, directory, options, image_base);
}

build_result get_built_size(const tls_directory_details& directory,
	const builder_options& options) noexcept
{
	return get_built_size_impl(directory, options);
}

build_result get_built_size(const tls_directory& directory,
	const builder_options& options) noexcept
{
	return get_built_size_impl(directory, options);
}

} //namespace pe_bliss::tls
