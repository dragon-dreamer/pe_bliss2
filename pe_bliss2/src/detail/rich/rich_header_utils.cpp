#include "pe_bliss2/detail/rich/rich_header_utils.h"

#include <algorithm>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iterator>

#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/detail/packed_serialization.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/rich/rich_header_loader.h"

#include "utilities/math.h"

namespace
{
std::size_t get_compid_offset(std::size_t dans_offset) noexcept
{
	return utilities::math::align_up(dans_offset +
		sizeof(pe_bliss::detail::rich::rich_header_utils::dans_signature),
		pe_bliss::detail::rich::rich_header_utils::compid_alignment);
}
} //namespace

namespace pe_bliss::detail::rich
{

std::size_t rich_header_utils::find_checksum(
	buffers::input_buffer_stateful_wrapper_ref& buffer)
{
	auto size = buffer.size();
	auto pos = size;
	auto min_pos = buffer.rpos();

	std::byte value{};
	auto it = rich_signature.crbegin();
	while (it != rich_signature.crend() && pos != min_pos)
	{
		buffer.set_rpos(--pos);
		if (buffer.read(sizeof(value), &value) != sizeof(value))
			return 0;

		if (*it == value)
			++it;
		else
			it = rich_signature.crbegin();
	}

	if (it != rich_signature.crend())
		return 0;

	auto offset = pos + rich_signature.size();
	if (size - offset < sizeof(checksum_type)
		|| (offset % sizeof(checksum_type)))
	{
		return 0;
	}

	return offset;
}

rich_header_utils::checksum_type rich_header_utils::decode_checksum(
	buffers::input_buffer_stateful_wrapper_ref& buffer)
{
	checksum_type result{};
	std::array<std::byte, 4u> buf{};
	buffer.read(sizeof(result), buf.data());
	detail::packed_serialization<>::deserialize(result, buf.data());
	return result;
}

std::size_t rich_header_utils::find_dans_signature(
	buffers::input_buffer_stateful_wrapper_ref& buffer,
	checksum_type checksum)
{
	std::uint32_t dans{};
	static constexpr std::size_t dans_size = sizeof(dans);
	std::array<std::byte, dans_size> bytes{};
	auto pos = buffer.rpos();
	while (pos >= dans_size)
	{
		pos -= dans_size;
		buffer.set_rpos(pos);
		if (buffer.read(bytes.size(), bytes.data()) != bytes.size())
		{
			throw pe_error(
				pe_bliss::rich::rich_header_loader_errc::no_dans_signature);
		}
		detail::packed_serialization<>::deserialize(dans, bytes.data());
		if ((dans ^ checksum) == dans_signature
			&& utilities::math::is_aligned<dans_alignment>(pos))
		{
			return pos;
		}
	}
	throw pe_error(
		pe_bliss::rich::rich_header_loader_errc::no_dans_signature);
}

void rich_header_utils::decode_compids(
	buffers::input_buffer_stateful_wrapper_ref& buffer,
	std::size_t checksum_pos, checksum_type checksum,
	std::vector<pe_bliss::rich::rich_compid>& compids)
{
	auto compid_pos = get_compid_offset(buffer.rpos());

	auto rich_signature_pos = checksum_pos - rich_signature.size();
	if ((rich_signature_pos - compid_pos) % rich_compid_size)
	{
		throw pe_error(
			pe_bliss::rich::rich_header_loader_errc::unable_to_decode_compids);
	}

	std::array<std::byte, rich_compid_size> data{};
	buffer.set_rpos(compid_pos);
	while (compid_pos + rich_compid_size <= rich_signature_pos)
	{
		auto& compid = compids.emplace_back();
		compid_pos += rich_compid_size;
		if (buffer.read(data.size(), data.data()) != data.size())
		{
			throw pe_error(
				pe_bliss::rich::rich_header_loader_errc::unable_to_decode_compids);
		}

		detail::packed_serialization<>::deserialize(compid, data.data());
		compid.use_count ^= checksum;
		compid.prod_id ^= checksum >> (CHAR_BIT * sizeof(compid.build_number));
		compid.build_number ^= checksum;
	}
}

} //namespace pe_bliss::detail::rich
