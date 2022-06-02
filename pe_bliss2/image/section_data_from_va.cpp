#include "pe_bliss2/image/section_data_from_va.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>

#include "buffers/input_buffer_section.h"

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/image_section_search.h"
#include "pe_bliss2/pe_error.h"

#include "utilities/generic_error.h"
#include "utilities/math.h"

namespace
{

using namespace pe_bliss;
using namespace pe_bliss::image;

template<bool IsConst>
struct data_result
{
	using buffer_ref_type = std::conditional_t<IsConst,
		const buffers::ref_buffer&, buffers::ref_buffer&>;
	buffer_ref_type buffer;
	std::size_t data_offset;
	std::size_t data_size;
};

template<typename Image>
data_result<std::is_const_v<Image>> section_data_from_rva_impl(Image& instance,
	rva_type rva, std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	if (!utilities::math::is_sum_safe(rva, data_size))
		throw pe_error(utilities::generic_errc::integer_overflow);

	if (include_headers && (rva + data_size <= instance.get_full_headers_buffer().size()))
		return { instance.get_full_headers_buffer(), rva, data_size };

	auto [header_it, data_it] = section_from_rva(instance, rva, data_size);
	if (data_it == std::cend(instance.get_section_data_list()))
		throw pe_error(image_errc::section_data_does_not_exist);

	std::uint32_t data_offset = rva - header_it->get_rva();
	if (data_offset + data_size <= data_it->size())
		return { data_it->get_buffer(), data_offset, data_size};

	if (allow_virtual_data)
	{
		data_size = data_offset < data_it->size()
			? static_cast<std::uint32_t>(data_it->size()) - data_offset
			: 0u;
		return { data_it->get_buffer(), (std::min)(data_it->size(),
			static_cast<std::size_t>(data_offset)), data_size };
	}

	throw pe_error(image_errc::section_data_does_not_exist);
}

template<typename Image>
data_result<std::is_const_v<Image>> section_data_from_rva_impl(
	Image& instance, rva_type rva, bool include_headers)
{
	auto& full_headers_buffer = instance.get_full_headers_buffer();
	if (include_headers && rva < full_headers_buffer.size())
		return { full_headers_buffer, rva, full_headers_buffer.size() - rva };

	auto [header_it, data_it] = section_from_rva(instance, rva, 1u);
	if (data_it == std::cend(instance.get_section_data_list()))
		throw pe_error(image_errc::section_data_does_not_exist);

	std::uint32_t data_offset = rva - header_it->get_rva();
	return { data_it->get_buffer(), data_offset, data_it->size() - data_offset };
}

} //namespace

namespace pe_bliss::image
{

buffers::input_buffer_ptr section_data_from_rva(const image& instance, rva_type rva,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	auto result = section_data_from_rva_impl(instance, rva, data_size,
		include_headers, allow_virtual_data);
	return buffers::reduce(result.buffer.data(), result.data_offset, result.data_size);
}

std::span<std::byte> section_data_from_rva(image& instance, rva_type rva,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	auto result = section_data_from_rva_impl(instance, rva, data_size,
		include_headers, allow_virtual_data);
	return { result.buffer.copied_data().data() + result.data_offset, result.data_size };
}

buffers::input_buffer_ptr section_data_from_va(const image& instance, std::uint32_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(instance, address_converter(instance).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

std::span<std::byte> section_data_from_va(image& instance, std::uint32_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(instance, address_converter(instance).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

buffers::input_buffer_ptr section_data_from_va(const image& instance, std::uint64_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(instance, address_converter(instance).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

std::span<std::byte> section_data_from_va(image& instance, std::uint64_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(instance, address_converter(instance).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

buffers::input_buffer_ptr section_data_from_rva(const image& instance, rva_type rva,
	bool include_headers)
{
	auto result = section_data_from_rva_impl(instance, rva, include_headers);
	return buffers::reduce(result.buffer.data(), result.data_offset, result.data_size);
}

std::span<std::byte> section_data_from_rva(image& instance, rva_type rva,
	bool include_headers)
{
	auto result = section_data_from_rva_impl(instance, rva, include_headers);
	return { result.buffer.copied_data().data() + result.data_offset, result.data_size };
}

buffers::input_buffer_ptr section_data_from_va(const image& instance, std::uint32_t va,
	bool include_headers)
{
	return section_data_from_rva(instance,
		address_converter(instance).va_to_rva(va), include_headers);
}

std::span<std::byte> section_data_from_va(image& instance, std::uint32_t va,
	bool include_headers)
{
	return section_data_from_rva(instance,
		address_converter(instance).va_to_rva(va), include_headers);
}

buffers::input_buffer_ptr section_data_from_va(const image& instance, std::uint64_t va,
	bool include_headers)
{
	return section_data_from_rva(instance,
		address_converter(instance).va_to_rva(va), include_headers);
}

std::span<std::byte> section_data_from_va(image& instance, std::uint64_t va,
	bool include_headers)
{
	return section_data_from_rva(instance,
		address_converter(instance).va_to_rva(va), include_headers);
}

} //namespace pe_bliss::image
