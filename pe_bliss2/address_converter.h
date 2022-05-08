#pragma once

#include <cstdint>
#include <system_error>
#include <type_traits>
#include <utility>

#include "pe_bliss2/detail/concepts.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss
{

enum class address_converter_errc
{
	address_conversion_overflow = 1
};

std::error_code make_error_code(address_converter_errc) noexcept;

class image;

namespace core
{
class optional_header;
} //namespace core

class address_converter
{
public:
	explicit address_converter(const image& instance) noexcept;
	explicit address_converter(const core::optional_header& header) noexcept;
	explicit address_converter(std::uint64_t image_base) noexcept;

public:
	template<detail::executable_pointer Pointer>
	[[nodiscard]] Pointer rva_to_va(rva_type rva) const
	{
		Pointer result{};
		rva_to_va(rva, result);
		return result;
	}

	void rva_to_va(rva_type rva, std::uint32_t& va) const;
	void rva_to_va(rva_type rva, std::uint64_t& va) const;

	[[nodiscard]] rva_type va_to_rva(std::uint32_t va) const;
	[[nodiscard]] rva_type va_to_rva(std::uint64_t va) const;

private:
	std::uint64_t image_base_;
};

} //namespace pe_bliss

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::address_converter_errc> : true_type {};
} //namespace std
