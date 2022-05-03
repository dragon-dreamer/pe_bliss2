#include "pe_bliss2/rich/rich_header_builder.h"

#include <exception>
#include <string>
#include <system_error>

#include "buffers/output_buffer_interface.h"

#include "pe_bliss2/pe_error.h"
#include "pe_bliss2/rich/rich_header.h"
#include "pe_bliss2/detail/packed_buffer_serialization.h"
#include "pe_bliss2/detail/rich/rich_header_utils.h"

#include "utilities/generic_error.h"
#include "utilities/math.h"
#include "utilities/safe_uint.h"

namespace
{

struct rich_header_builder_error_category : std::error_category
{
	const char* name() const noexcept override
	{
		return "rich_header_builder";
	}

	std::string message(int ev) const override
	{
		using enum pe_bliss::rich::rich_header_builder_errc;
		switch (static_cast<pe_bliss::rich::rich_header_builder_errc>(ev))
		{
		case unable_to_build_rich_header:
			return "Unable to build Rich header";
		default:
			return {};
		}
	}
};

const rich_header_builder_error_category rich_header_builder_error_category_instance;

} //namespace

namespace pe_bliss::rich
{

std::error_code make_error_code(rich_header_builder_errc e) noexcept
{
	return { static_cast<int>(e), rich_header_builder_error_category_instance };
}

void build(const rich_header& header,
	buffers::output_buffer_interface& buffer) try
{
	std::size_t aligned_buffer_offset = header.get_dos_stub_offset();
	if (!utilities::math::align_up_if_safe(aligned_buffer_offset,
		detail::rich::rich_header_utils::compid_alignment))
	{
		throw pe_error(utilities::generic_errc::integer_overflow);
	}

	buffer.set_wpos(aligned_buffer_offset);

	const auto checksum = header.get_checksum();
	detail::packed_buffer_serialization<>::serialize(
		detail::rich::rich_header_utils::dans_signature ^ checksum, buffer);
	for (std::size_t i = 0; i != 3; ++i) //padding
		detail::packed_buffer_serialization<>::serialize(checksum, buffer);

	for (auto compid : header.get_compids())
	{
		compid.use_count ^= checksum;
		compid.prod_id ^= checksum >> (CHAR_BIT * sizeof(compid.build_number));
		compid.build_number ^= checksum;
		detail::packed_buffer_serialization<>::serialize(compid, buffer);
	}

	buffer.write(detail::rich::rich_header_utils::rich_signature.size(),
		detail::rich::rich_header_utils::rich_signature.data());
	detail::packed_buffer_serialization<>::serialize(checksum, buffer);
}
catch (...)
{
	std::throw_with_nested(pe_error(rich_header_builder_errc::unable_to_build_rich_header));
}

std::size_t get_built_size(const rich_header& header)
{
	std::size_t aligned_offset = header.get_dos_stub_offset();
	if (!utilities::math::align_up_if_safe(aligned_offset,
		detail::rich::rich_header_utils::compid_alignment))
	{
		throw pe_error(rich_header_errc::invalid_rich_header_offset);
	}

	utilities::safe_uint size(aligned_offset - header.get_dos_stub_offset());
	try
	{
		size += sizeof(detail::rich::rich_header_utils::dans_signature);
		size += sizeof(std::uint32_t) * 3; //Padding
		size += header.get_compids().size()
			* detail::rich::rich_header_utils::rich_compid_size;
		size += detail::rich::rich_header_utils::rich_signature.size();
		size += sizeof(detail::rich::rich_header_utils::checksum_type);
	}
	catch (...)
	{
		std::throw_with_nested(pe_error(rich_header_errc::invalid_rich_header_offset));
	}

	if (!utilities::math::is_sum_safe(size.value(), header.get_dos_stub_offset()))
		throw pe_error(rich_header_errc::invalid_rich_header_offset);

	return size.value();
}

} // namespace pe_bliss::rich
