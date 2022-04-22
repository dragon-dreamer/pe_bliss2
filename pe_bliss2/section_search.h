#pragma once

#include <cstddef>
#include <cstdint>

#include "pe_bliss2/pe_types.h"

namespace pe_bliss
{
class section_header;
} //namespace pe_bliss

namespace pe_bliss::section_search
{

struct [[nodiscard]] by_raw_offset
{
public:
	by_raw_offset(std::uint32_t raw_offset, std::uint32_t section_alignment,
		std::uint32_t data_size = 0);

	//Section raw position + raw size must not overflow
	[[nodiscard]]
	bool operator()(const section_header& header) const noexcept;

private:
	std::uint32_t raw_offset_;
	std::uint32_t section_alignment_;
	std::uint32_t data_size_;
};

struct [[nodiscard]] by_rva
{
public:
	by_rva(rva_type rva, std::uint32_t section_alignment,
		std::uint32_t data_size = 0);

	//Section RVA + virtual size must not overflow
	[[nodiscard]]
	bool operator()(const section_header& header) const noexcept;

private:
	rva_type rva_;
	std::uint32_t section_alignment_;
	std::uint32_t data_size_;
};

struct [[nodiscard]] by_pointer
{
public:
	explicit by_pointer(const section_header* ptr) noexcept;

	[[nodiscard]]
	bool operator()(const section_header& header) const noexcept;

private:
	const section_header* ptr_;
};

} //namespace pe_bliss::section_search
