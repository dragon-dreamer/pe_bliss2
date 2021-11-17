#pragma once

#include <cstddef>
#include <cstdint>

#include "pe_bliss2/section_header.h"
#include "pe_bliss2/pe_types.h"

namespace pe_bliss::section_search
{

struct by_raw_offset
{
public:
	by_raw_offset(std::uint32_t raw_offset, std::uint32_t section_alignment,
		std::uint32_t data_size = 0);

	//Section raw position + raw size must not overflow
	[[nodiscard]]
	bool operator()(const section_header& header) const;

private:
	std::uint32_t raw_offset_;
	std::uint32_t section_alignment_;
	std::uint32_t data_size_;
};

struct by_rva
{
public:
	by_rva(rva_type rva, std::uint32_t section_alignment,
		std::uint32_t data_size = 0);

	//Section RVA + virtual size must not overflow
	[[nodiscard]]
	bool operator()(const section_header& header) const;

private:
	rva_type rva_;
	std::uint32_t section_alignment_;
	std::uint32_t data_size_;
};

struct by_pointer
{
public:
	explicit by_pointer(const section_header* ptr);

	[[nodiscard]]
	bool operator()(const section_header& header) const noexcept;

private:
	const section_header* ptr_;
};

} //namespace pe_bliss::section_search
