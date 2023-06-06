#include "pe_bliss2/rich/rich_compid.h"

#include <bit>
#include <climits>
#include <cstdint>

namespace pe_bliss::rich
{

std::uint32_t get_checksum(const rich_compid& compid) noexcept
{
	return std::rotl(
		static_cast<std::uint32_t>(compid.build_number)
		| (static_cast<std::uint32_t>(compid.prod_id)
			<< (sizeof(std::uint16_t) * CHAR_BIT)),
		static_cast<std::uint8_t>(compid.use_count));
}

} //namespace pe_bliss::rich
