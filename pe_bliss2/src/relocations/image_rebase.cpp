#include "pe_bliss2/relocations/image_rebase.h"

#include <cstdint>
#include <exception>
#include <string>
#include <system_error>

#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/struct_from_va.h"
#include "pe_bliss2/image/struct_to_va.h"
#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/math.h"

namespace
{

struct rebase_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "rebase";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::relocations::rebase_errc;
		switch (static_cast<pe_bliss::relocations::rebase_errc>(ev))
		{
		case unable_to_rebase_inexistent_data:
			return "Unable to rebase inexistent data";
		case invalid_relocation_address:
			return "Invalid relocation virtual address";
		default:
			return {};
		}
	}
};

const rebase_error_category rebase_error_category_instance;

using namespace pe_bliss;
using namespace pe_bliss::relocations;

template<typename T, typename Reloc>
void process_relocation(image::image& instance, rva_type rva,
	std::uint64_t base_diff, const Reloc& entry, bool ignore_virtual_data)
{
	if (!utilities::math::add_if_safe<rva_type>(rva, entry.get_address()))
		throw pe_error(rebase_errc::invalid_relocation_address);

	packed_struct<T> value;
	try
	{
		struct_from_rva(instance, rva, value, true, ignore_virtual_data);
	}
	catch (const std::system_error&)
	{
		std::throw_with_nested(
			pe_error(rebase_errc::unable_to_rebase_inexistent_data));
	}

	auto machine = instance.get_file_header().get_machine_type();
	value.get() = static_cast<T>(entry.apply_to(value.get(), base_diff, machine));
	struct_to_rva(instance, rva, value, true, !ignore_virtual_data);
}

template<typename RelocsList>
void rebase_impl(image::image& instance, const RelocsList& relocs,
	const rebase_options& options)
{
	const auto base_diff = options.new_base
		- instance.get_optional_header().get_raw_image_base();
	if (!base_diff)
		return;

	auto machine = instance.get_file_header().get_machine_type();
	for (const auto& basereloc : relocs)
	{
		for (const auto& entry : basereloc.get_relocations())
		{
			// Make sure all relocation entries are supported
			// before applying relocations
			[[maybe_unused]] auto size = entry.get_affected_size_in_bytes(machine);
		}
	}

	for (const auto& basereloc : relocs)
	{
		auto base_rva = basereloc.get_descriptor()->virtual_address;
		for (const auto& entry : basereloc.get_relocations())
		{
			switch (entry.get_affected_size_in_bytes(machine))
			{
				case sizeof(std::uint16_t):
					process_relocation<std::uint16_t>(instance,
						base_rva, base_diff, entry, options.ignore_virtual_data);
					break;
				case sizeof(std::uint32_t):
					process_relocation<std::uint32_t>(instance,
						base_rva, base_diff, entry, options.ignore_virtual_data);
					break;
				case sizeof(std::uint64_t):
					process_relocation<std::uint64_t>(instance,
						base_rva, base_diff, entry, options.ignore_virtual_data);
					break;
				default:
					break;
			}
		}
	}

	instance.get_optional_header().set_raw_image_base(options.new_base);
}

} //namespace

namespace pe_bliss::relocations
{

std::error_code make_error_code(rebase_errc e) noexcept
{
	return { static_cast<int>(e), rebase_error_category_instance };
}

void rebase(image::image& instance, const base_relocation_details_list& relocs,
	const rebase_options& options)
{
	rebase_impl(instance, relocs, options);
}

void rebase(image::image& instance, const base_relocation_list& relocs,
	const rebase_options& options)
{
	rebase_impl(instance, relocs, options);
}

} //namespace pe_bliss::relocations
