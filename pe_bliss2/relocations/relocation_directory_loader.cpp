#include "pe_bliss2/relocations/relocation_directory_loader.h"

#include <cstdint>
#include <system_error>

#include "pe_bliss2/core/data_directories.h"
#include "pe_bliss2/detail/relocations/image_base_relocation.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/pe_types.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{

struct relocation_directory_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "relocation_directory_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::relocations::relocation_directory_loader_errc;
		switch (static_cast<pe_bliss::relocations::relocation_directory_loader_errc>(ev))
		{
		case invalid_relocation_block_size:
			return "Invalid base relocation block size";
		case unaligned_relocation_entry:
			"Unaligned base relocation entry";
		case invalid_directory_size:
			return "Invalid relocation directory size";
		case invalid_relocation_entry:
			return "Invalid relocation entry";
		default:
			return {};
		}
	}
};

const relocation_directory_loader_error_category relocation_directory_loader_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::relocations;

using safe_rva_type = utilities::safe_uint<rva_type>;

bool load_element(const image::image& instance, const loader_options& options,
	base_relocation_details::entry_list_type& relocations, std::uint32_t& elem_count,
	safe_rva_type& current_rva, rva_type last_rva)
{
	auto& elem = relocations.emplace_back();
	auto& elem_descriptor = elem.get_descriptor();

	if (!utilities::math::is_sum_safe<rva_type>(last_rva, elem_descriptor.packed_size)
		|| current_rva + elem_descriptor.packed_size > last_rva)
	{
		elem.add_error(relocation_entry_errc::invalid_relocation_entry);
		return false;
	}

	try
	{
		struct_from_rva(instance, current_rva.value(), elem_descriptor,
			options.include_headers, options.allow_virtual_data);
		current_rva += elem_descriptor.packed_size;
	}
	catch (const std::system_error&)
	{
		elem.add_error(relocation_entry_errc::invalid_relocation_entry);
		return false;
	}

	--elem_count;

	try
	{
		//Check relocation type is supported
		[[maybe_unused]] auto size = elem.get_affected_size_in_bytes();
	}
	catch (const pe_error& e)
	{
		elem.add_error(e.code());
	}
	
	if (elem.requires_parameter())
	{
		bool has_space =
			utilities::math::is_sum_safe<rva_type>(current_rva.value(),
				sizeof(detail::relocations::type_or_offset_entry))
			&& current_rva + sizeof(detail::relocations::type_or_offset_entry) <= last_rva;
		if (elem_count && has_space)
		{
			try
			{
				struct_from_rva(instance, current_rva.value(), elem.get_param().emplace(),
					options.include_headers, options.allow_virtual_data);
				current_rva += sizeof(detail::relocations::type_or_offset_entry);
				--elem_count;
			}
			catch (const std::system_error&)
			{
				elem.get_param().reset();
				elem.add_error(relocation_entry_errc::relocation_param_is_absent);
				return false;
			}
		}
		else
		{
			elem.add_error(relocation_entry_errc::relocation_param_is_absent);
		}
		return has_space;
	}
	
	return true;
}

bool load_elements(const image::image& instance, const loader_options& options,
	base_relocation_details::entry_list_type& relocations, std::uint32_t& elem_count,
	safe_rva_type& current_rva, rva_type last_rva)
{
	while (elem_count)
	{
		if (!load_element(instance, options, relocations, elem_count, current_rva, last_rva))
			return false;
	}
	return true;
}

} //namespace

namespace pe_bliss::relocations
{

std::error_code make_error_code(relocation_directory_loader_errc e) noexcept
{
	return { static_cast<int>(e), relocation_directory_loader_error_category_instance };
}

std::optional<relocation_directory> load(const image::image& instance,
	const loader_options& options)
{
	std::optional<relocation_directory> result;
	if (!instance.get_data_directories().has_reloc())
		return result;

	const auto& reloc_dir_info = instance.get_data_directories().get_directory(
		core::data_directories::directory_type::basereloc);

	auto& list = result.emplace().relocations;

	safe_rva_type current_rva = reloc_dir_info->virtual_address;
	auto last_rva = current_rva.value();
	if (!utilities::math::add_if_safe(last_rva, reloc_dir_info->size))
	{
		result->errors.add_error(relocation_directory_loader_errc::invalid_directory_size);
		return result;
	}

	while (current_rva < last_rva)
	{
		auto& basereloc = list.emplace_back();
		auto& descriptor = basereloc.get_descriptor();
		try
		{
			struct_from_rva(instance, current_rva.value(), descriptor,
				options.include_headers, options.allow_virtual_data);

			auto aligned_rva = current_rva;
			aligned_rva.align_up(sizeof(rva_type));
			if (current_rva != aligned_rva)
				basereloc.add_error(relocation_directory_loader_errc::unaligned_relocation_entry);

			current_rva += descriptor.packed_size;
			if (descriptor->size_of_block < descriptor.packed_size)
			{
				basereloc.add_error(relocation_directory_loader_errc::invalid_relocation_block_size);
				continue;
			}
		}
		catch (const std::system_error&)
		{
			basereloc.add_error(relocation_directory_loader_errc::invalid_relocation_entry);
			return result;
		}

		auto elem_count = static_cast<std::uint32_t>(descriptor->size_of_block
			- descriptor.packed_size);
		if ((elem_count % 2))
		{
			current_rva += elem_count;
			basereloc.add_error(relocation_directory_loader_errc::invalid_relocation_block_size);
			continue;
		}

		elem_count /= 2;
		auto& relocations = basereloc.get_relocations();
		if (!load_elements(instance, options, relocations, elem_count, current_rva, last_rva))
			return result;
	}

	if (current_rva != last_rva)
		result->errors.add_error(relocation_directory_loader_errc::invalid_directory_size);

	return result;
}

} //namespace pe_bliss::relocations
