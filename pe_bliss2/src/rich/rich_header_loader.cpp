#include "pe_bliss2/rich/rich_header_loader.h"

#include <cstddef>
#include <optional>
#include <string>
#include <system_error>

#include "buffers/input_buffer_stateful_wrapper.h"

#include "pe_bliss2/detail/rich/rich_header_utils.h"
#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/rich/rich_header.h"

namespace
{

struct rich_header_loader_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "rich_header_loader";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::rich::rich_header_loader_errc;
		switch (static_cast<pe_bliss::rich::rich_header_loader_errc>(ev))
		{
		case no_dans_signature:
			return "No rich header DanS signature";
		case unable_to_decode_compids:
			return "Unable to decode rich header COMPIDs";
		default:
			return {};
		}
	}
};

const rich_header_loader_error_category rich_header_loader_error_category_instance;

} //namespace

namespace pe_bliss::rich
{

std::error_code make_error_code(rich_header_loader_errc e) noexcept
{
	return { static_cast<int>(e), rich_header_loader_error_category_instance };
}

std::optional<rich_header> load(buffers::input_buffer_stateful_wrapper_ref& buffer)
{
	std::optional<rich_header> result;
	auto checksum_pos = detail::rich::rich_header_utils::find_checksum(buffer);
	if (!checksum_pos)
		return result;

	auto& header = result.emplace();
	buffer.set_rpos(checksum_pos);
	header.set_checksum(detail::rich::rich_header_utils::decode_checksum(buffer));

	auto dans_offset = detail::rich::rich_header_utils::find_dans_signature(
		buffer, header.get_checksum());
	header.set_dos_stub_offset(dans_offset, dans_offset
		+ buffer.get_buffer().absolute_offset());

	buffer.set_rpos(header.get_dos_stub_offset());
	detail::rich::rich_header_utils::decode_compids(
		buffer, checksum_pos, header.get_checksum(),
		header.get_compids());

	return result;
}

} // namespace pe_bliss::rich
