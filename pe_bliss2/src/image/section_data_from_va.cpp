#include "pe_bliss2/image/section_data_from_va.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <iterator>
#include <type_traits>

#include "buffers/input_buffer_section.h"
#include "buffers/input_virtual_buffer.h"

#include "pe_bliss2/address_converter.h"
#include "pe_bliss2/image/image.h"
#include "pe_bliss2/image/image_errc.h"
#include "pe_bliss2/image/image_section_search.h"
#include "pe_bliss2/pe_error.h"

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
	std::size_t additional_virtual_size;
};

template<typename Image, typename RefBuffer>
data_result<std::is_const_v<Image>> to_data_result(
	Image& /* instance */, rva_type rva, rva_type section_rva, RefBuffer& buffer)
{
	auto physical_size = buffer.physical_size();
	auto total_size = buffer.size();
	std::uint32_t data_offset = rva - section_rva;
	std::size_t available_physical_size = physical_size < data_offset
		? 0u : physical_size - data_offset;
	std::size_t available_total_size = total_size < data_offset
		? 0u : total_size - data_offset;

	return { buffer, data_offset,
		available_physical_size, available_total_size - available_physical_size };
}

template<typename Image>
data_result<std::is_const_v<Image>> section_data_from_rva_impl(
	Image& instance, rva_type rva, bool include_headers)
{
	auto& full_headers_buffer = instance.get_full_headers_buffer();
	if (rva < full_headers_buffer.size())
	{
		if (!include_headers)
			throw pe_error(image_errc::section_data_does_not_exist);

		return to_data_result(instance, rva, 0u, full_headers_buffer);
	}

	auto [header_it, data_it] = section_from_rva(instance, rva, 1u);
	if (data_it == std::cend(instance.get_section_data_list()))
	{
		auto [empty_header_it, empty_data_it] = section_from_rva(instance, rva, 0u);
		if (empty_data_it == std::cend(instance.get_section_data_list()))
			throw pe_error(image_errc::section_data_does_not_exist);

		return { empty_data_it->get_buffer(), empty_data_it->size(), 0u, 0u };
	}

	return to_data_result(instance, rva, header_it->get_rva(), data_it->get_buffer());
}

template<typename Image>
data_result<std::is_const_v<Image>> section_data_from_rva_impl(Image& instance,
	rva_type rva, std::uint32_t data_size, bool include_headers)
{
	auto& full_headers_buffer = instance.get_full_headers_buffer();
	if (rva < full_headers_buffer.size())
	{
		if (!utilities::math::is_sum_safe(rva, data_size))
			throw pe_error(image_errc::section_data_does_not_exist);
		if (!include_headers || full_headers_buffer.size() < rva + data_size)
			throw pe_error(image_errc::section_data_does_not_exist);

		return to_data_result(instance, rva, 0u, full_headers_buffer);
	}

	auto [header_it, data_it] = section_from_rva(instance, rva, data_size);
	if (data_it == std::cend(instance.get_section_data_list()))
		throw pe_error(image_errc::section_data_does_not_exist);

	return to_data_result(instance, rva, header_it->get_rva(), data_it->get_buffer());
}

} //namespace

namespace pe_bliss::image
{

buffers::input_buffer_ptr section_data_from_rva(const image& instance, rva_type rva,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	auto result = section_data_from_rva_impl(instance, rva, data_size,
		include_headers);
	if (!allow_virtual_data && result.data_size < data_size)
		throw pe_error(image_errc::section_data_does_not_exist);
	if (result.data_size + result.additional_virtual_size < data_size)
		throw pe_error(image_errc::section_data_does_not_exist);

	return buffers::reduce(result.buffer.data(), result.data_offset, data_size);
}

std::span<std::byte> section_data_from_rva(image& instance, rva_type rva,
	std::uint32_t data_size, bool include_headers)
{
	auto result = section_data_from_rva_impl(instance, rva, data_size,
		include_headers);
	if (result.data_size < data_size)
		throw pe_error(image_errc::section_data_does_not_exist);
	return { result.buffer.copied_data().data() + result.data_offset, data_size };
}

buffers::input_buffer_ptr section_data_from_va(const image& instance, std::uint32_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(instance, address_converter(instance).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

std::span<std::byte> section_data_from_va(image& instance, std::uint32_t va,
	std::uint32_t data_size, bool include_headers)
{
	return section_data_from_rva(instance, address_converter(instance).va_to_rva(va),
		data_size, include_headers);
}

buffers::input_buffer_ptr section_data_from_va(const image& instance, std::uint64_t va,
	std::uint32_t data_size, bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(instance, address_converter(instance).va_to_rva(va),
		data_size, include_headers, allow_virtual_data);
}

std::span<std::byte> section_data_from_va(image& instance, std::uint64_t va,
	std::uint32_t data_size, bool include_headers)
{
	return section_data_from_rva(instance, address_converter(instance).va_to_rva(va),
		data_size, include_headers);
}

buffers::input_buffer_ptr section_data_from_rva(const image& instance, rva_type rva,
	bool include_headers, bool allow_virtual_data)
{
	auto result = section_data_from_rva_impl(instance, rva, include_headers);
	auto size = result.data_size;
	if (allow_virtual_data)
		size += result.additional_virtual_size;
	return buffers::reduce(result.buffer.data(), result.data_offset, size);
}

std::span<std::byte> section_data_from_rva(image& instance, rva_type rva,
	bool include_headers)
{
	auto result = section_data_from_rva_impl(instance, rva, include_headers);
	return { result.buffer.copied_data().data() + result.data_offset, result.data_size };
}

buffers::input_buffer_ptr section_data_from_va(const image& instance, std::uint32_t va,
	bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(instance,
		address_converter(instance).va_to_rva(va), include_headers,
		allow_virtual_data);
}

std::span<std::byte> section_data_from_va(image& instance, std::uint32_t va,
	bool include_headers)
{
	return section_data_from_rva(instance,
		address_converter(instance).va_to_rva(va), include_headers);
}

buffers::input_buffer_ptr section_data_from_va(const image& instance, std::uint64_t va,
	bool include_headers, bool allow_virtual_data)
{
	return section_data_from_rva(instance,
		address_converter(instance).va_to_rva(va), include_headers,
		allow_virtual_data);
}

std::span<std::byte> section_data_from_va(image& instance, std::uint64_t va,
	bool include_headers)
{
	return section_data_from_rva(instance,
		address_converter(instance).va_to_rva(va), include_headers);
}

} //namespace pe_bliss::image
