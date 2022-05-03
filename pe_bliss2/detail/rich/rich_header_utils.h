#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "pe_bliss2/detail/packed_reflection.h"
#include "pe_bliss2/rich/rich_compid.h"

#include "utilities/static_class.h"

namespace buffers
{
class input_buffer_interface;
} //namespace buffers

namespace pe_bliss::detail::rich
{

class rich_header_utils final : utilities::static_class
{
public:
	using checksum_type = std::uint32_t;

public:
	[[nodiscard]]
	static std::size_t find_checksum(
		buffers::input_buffer_interface& buffer);

	//Buffer should point to checksum bytes.
	[[nodiscard]]
	static checksum_type decode_checksum(
		buffers::input_buffer_interface& buffer);

	//Buffer should point to end of checksum bytes
	[[nodiscard]]
	static std::size_t find_dans_signature(
		buffers::input_buffer_interface& buffer,
		checksum_type checksum);

	//Buffer should point to dans signature
	static void decode_compids(buffers::input_buffer_interface& buffer,
		std::size_t checksum_pos, checksum_type checksum,
		std::vector<pe_bliss::rich::rich_compid>& compids);

public:
	static constexpr std::uint32_t dans_alignment = 16u;
	static constexpr std::uint32_t compid_alignment = 16u;
	static constexpr std::uint32_t dans_signature = 0x536e6144u; //'DanS'
	static constexpr std::array rich_signature{
		std::byte{'R'}, std::byte{'i'}, std::byte{'c'}, std::byte{'h'} };
	static constexpr auto rich_compid_size
		= detail::packed_reflection::get_type_size<pe_bliss::rich::rich_compid>();
};

} //namespace pe_bliss::detail::rich
