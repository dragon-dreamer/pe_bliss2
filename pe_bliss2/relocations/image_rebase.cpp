#include "pe_bliss2/relocations/image_rebase.h"

#include <cstdint>

#include "pe_bliss2/packed_struct.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/pe_error.h"
#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::relocations;

template<typename T, typename Reloc>
void process_relocation(image::image& instance, rva_type rva, std::uint64_t base_diff, const Reloc& entry)
{
	if (!utilities::math::add_if_safe<rva_type>(rva, entry.get_address()))
		throw pe_error(utilities::generic_errc::integer_overflow);

	packed_struct<T> value;
	instance.struct_from_rva(rva, value, true, false);
	value.get() = static_cast<T>(entry.apply_to(value.get(), base_diff));
	instance.struct_to_rva(rva, value, true, true);
}

template<typename RelocsList>
void rebase_impl(image::image& instance, const RelocsList& relocs,
	std::uint64_t new_base)
{
	const auto base_diff = new_base - instance.get_optional_header().get_raw_image_base();
	if (!base_diff)
		return;

	for (const auto& basereloc : relocs)
	{
		auto base_rva = basereloc.get_descriptor()->virtual_address;
		for (const auto& entry : basereloc.get_relocations())
		{
			switch (entry.get_affected_size_in_bytes())
			{
				case sizeof(std::uint16_t):
					process_relocation<std::uint16_t>(instance, base_rva, base_diff, entry);
					break;
				case sizeof(std::uint32_t):
					process_relocation<std::uint32_t>(instance, base_rva, base_diff, entry);
					break;
				case sizeof(std::uint64_t):
					process_relocation<std::uint64_t>(instance, base_rva, base_diff, entry);
					break;
				default:
					break;
			}
		}
	}

	instance.get_optional_header().set_raw_image_base(new_base);
}

} //namespace

namespace pe_bliss::relocations
{

void rebase(image::image& instance, const base_relocation_details_list& relocs, std::uint64_t new_base)
{
	rebase_impl(instance, relocs, new_base);
}

void rebase(image::image& instance, const base_relocation_list& relocs, std::uint64_t new_base)
{
	rebase_impl(instance, relocs, new_base);
}

} //namespace pe_bliss::relocations
