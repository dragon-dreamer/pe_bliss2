#include "pe_bliss2/image/image_loader.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iterator>
#include <limits>
#include <string>
#include <system_error>

#include "buffers/input_buffer_section.h"
#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/dos/dos_header.h"
#include "pe_bliss2/dos/dos_header_errc.h"
#include "pe_bliss2/dos/dos_header_validator.h"
#include "pe_bliss2/core/image_signature_validator.h"
#include "pe_bliss2/core/optional_header_validator.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/section/section_data.h"
#include "pe_bliss2/section/section_errc.h"
#include "pe_bliss2/section/section_table_validator.h"

#include "utilities/math.h"

namespace
{

struct image_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "image_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::image::image_loader_errc;
		switch (static_cast<pe_bliss::image::image_loader_errc>(ev))
		{
		case unable_to_load_full_headers_buffer:
			return "Unable to load full headers buffer";
		case unable_to_load_full_section_buffer:
			return "Unable to load full section buffer";
		default:
			return {};
		}
	}
};

const image_loader_error_category image_loader_error_category_instance;

} //namespace

namespace pe_bliss::image
{

std::error_code make_error_code(image_loader_errc e) noexcept
{
	return { static_cast<int>(e), image_loader_error_category_instance };
}

void image_loader::load(
	image& instance,
	error_list& warnings,
	std::exception_ptr& fatal_error,
	const buffers::input_buffer_ptr& buffer,
	const image_load_options& options)
{
	buffers::input_buffer_stateful_wrapper wrapper(buffer);

	auto buffer_size = wrapper.size();

	try
	{
		instance.set_loaded_to_memory(options.image_loaded_to_memory);

		auto& dos_hdr = instance.get_dos_header();
		dos_hdr.deserialize(wrapper, options.allow_virtual_headers);
		if (options.dos_header_validation.validate_magic)
			dos::validate_magic(dos_hdr).throw_on_error();
		if (options.dos_header_validation.validate_e_lfanew)
			dos::validate_e_lfanew(dos_hdr).throw_on_error();

		auto e_lfanew = dos_hdr.get_descriptor()->e_lfanew;
		if (e_lfanew < dos::dos_header::descriptor_type::packed_size)
		{
			wrapper.set_rpos(e_lfanew);
		}
		else
		{
			instance.get_dos_stub().deserialize(wrapper, {
				.copy_memory = options.eager_dos_stub_data_copy,
				.e_lfanew = e_lfanew
			});
		}

		instance.get_image_signature().deserialize(
			wrapper, options.allow_virtual_headers);
		if (options.validate_image_signature)
			core::validate(instance.get_image_signature()).throw_on_error();

		auto& file_hdr = instance.get_file_header();
		auto& optional_hdr = instance.get_optional_header();
		file_hdr.deserialize(wrapper, options.allow_virtual_headers);
		optional_hdr.deserialize(wrapper, options.allow_virtual_headers);
		if (options.validate_size_of_optional_header)
		{
			if (auto err = validate_size_of_optional_header(
				file_hdr.get_descriptor()->size_of_optional_header,
				optional_hdr); err)
			{
				warnings.add_error(err);
			}
		}
		core::validate(optional_hdr, options.optional_header_validation,
			file_hdr.is_dll(), warnings);

		instance.get_data_directories().deserialize(wrapper,
			optional_hdr.get_number_of_rva_and_sizes(), options.allow_virtual_headers);

		if (options.validate_image_base)
		{
			if (auto err = validate_image_base(optional_hdr,
				instance.has_relocations()); err)
			{
				warnings.add_error(err);
			}
		}

		try
		{
			wrapper.set_rpos(file_hdr.get_section_table_buffer_pos());
		}
		catch (const std::system_error&)
		{
			std::throw_with_nested(pe_error(section::section_errc::unable_to_read_section_table));
		}

		auto& section_tbl = instance.get_section_table();
		section_tbl.deserialize(wrapper, file_hdr.get_descriptor()->number_of_sections,
			options.allow_virtual_headers);

		const auto& section_headers = section_tbl.get_section_headers();
		if (options.validate_sections)
		{
			section::validate_section_headers(optional_hdr, section_headers, warnings);
		}

		if (options.load_section_data)
		{
			section::section_data_load_options load_opts{
				.section_alignment = optional_hdr.get_raw_section_alignment(),
				.copy_memory = options.eager_section_data_copy,
				.image_loaded_to_memory = options.image_loaded_to_memory
			};

			auto& sections = instance.get_section_data_list();
			for (auto it = section_headers.cbegin(),
				end = section_headers.cend(); it != end; ++it)
			{
				try
				{
					sections.emplace_back().deserialize(*it, wrapper, load_opts);
				}
				catch (const pe_error& e)
				{
					warnings.add_error(e.code(),
						std::distance(section_headers.cbegin(), it));
				}
			}
		}

		std::size_t first_section_data_offset = (std::numeric_limits<std::size_t>::max)();
		std::size_t last_section_data_offset = 0u;
		if (options.load_full_sections_buffer || options.load_full_headers_buffer)
		{
			const auto section_alignment = optional_hdr.get_raw_section_alignment();
			for (const auto& header : section_headers)
			{
				if (!header.get_pointer_to_raw_data())
					continue;

				first_section_data_offset = (std::min<std::size_t>)(first_section_data_offset,
					header.get_pointer_to_raw_data());
				last_section_data_offset = (std::max)(last_section_data_offset,
					static_cast<std::size_t>(header.get_pointer_to_raw_data())
					+ header.get_raw_size(section_alignment));
			}
		}

		if (options.load_full_sections_buffer && last_section_data_offset)
		{
			try
			{
				auto full_sections_buffer = buffers::reduce(buffer,
					first_section_data_offset, last_section_data_offset - first_section_data_offset);
				instance.get_full_sections_buffer().deserialize(std::move(full_sections_buffer),
					options.eager_full_sections_buffer_copy);
			}
			catch (...)
			{
				warnings.add_error(
					image_loader_errc::unable_to_load_full_section_buffer);
			}
		}

		if (options.load_overlay && !options.image_loaded_to_memory)
		{
			std::uint64_t section_data_end_offset = section_tbl.get_raw_data_end_offset(
				optional_hdr.get_raw_section_alignment());
			try
			{
				instance.get_overlay().deserialize(section_data_end_offset,
					optional_hdr.get_raw_size_of_headers(),
					0u, wrapper, options.eager_overlay_data_copy);
			}
			catch (const pe_error& e)
			{
				warnings.add_error(e.code());
			}
		}

		if (options.validate_size_of_image)
		{
			const auto* last_section = section_headers.empty()
				? nullptr : &section_headers.back();
			if (auto err = core::validate_size_of_image(last_section, optional_hdr); err)
				warnings.add_error(err);
		}

		if (options.load_full_headers_buffer)
		{
			std::size_t size = optional_hdr.get_raw_size_of_headers();
			if (last_section_data_offset)
			{
				const auto section_alignment = optional_hdr.get_raw_section_alignment();
				if (section_alignment && std::has_single_bit(section_alignment))
					(void)utilities::math::align_up_if_safe(size, section_alignment);
				size = std::min(size, first_section_data_offset);
			}

			size = std::min(size, buffer_size);
			try
			{
				instance.get_full_headers_buffer().deserialize(
					buffers::reduce(buffer, 0u, size),
					options.eager_full_headers_buffer_copy);
			}
			catch (...)
			{
				warnings.add_error(
					image_loader_errc::unable_to_load_full_headers_buffer);
			}
		}
	}
	catch (const std::system_error&)
	{
		fatal_error = std::current_exception();
	}
}

image_load_result image_loader::load(
	const buffers::input_buffer_ptr& buffer,
	const image_load_options& options)
{
	image_load_result result;
	load(result.image, result.warnings, result.fatal_error, buffer, options);
	return result;
}

} //namespace pe_bliss::image
