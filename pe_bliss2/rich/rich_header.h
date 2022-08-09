#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <system_error>
#include <type_traits>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/rich/rich_compid.h"

namespace buffers
{
class input_buffer_stateful_wrapper_ref;
} //namespace buffers

namespace pe_bliss::rich
{

enum class rich_header_errc
{
	invalid_rich_header_offset = 1,
	unaligned_rich_header_offset,
	unable_to_calculate_rich_signature
};

std::error_code make_error_code(rich_header_errc) noexcept;

class [[nodiscard]] rich_header
{
public:
	using compid_list = std::vector<rich_compid>;
	using checksum_type = std::uint32_t;

public:
	[[nodiscard]] compid_list& get_compids() & noexcept
	{
		return compids_;
	}

	[[nodiscard]] const compid_list& get_compids() const& noexcept
	{
		return compids_;
	}

	[[nodiscard]] compid_list get_compids() && noexcept
	{
		return std::move(compids_);
	}

	[[nodiscard]] checksum_type get_checksum() const noexcept
	{
		return checksum_;
	}

	[[nodiscard]] std::size_t get_dos_stub_offset() const noexcept
	{
		return dans_relative_offset_;
	}

	[[nodiscard]] std::size_t get_absolute_offset() const noexcept
	{
		return dans_absolute_offset_;
	}

	void set_dos_stub_offset(std::size_t relative_offset,
		std::size_t absolute_offset) noexcept
	{
		dans_relative_offset_ = relative_offset;
		dans_absolute_offset_ = absolute_offset;
	}

	void set_checksum(checksum_type checksum) noexcept
	{
		checksum_ = checksum;
	}

	//image_buffer is expected to contain data from the beginning of image
	//with already serialized DOS header and DOS stub.
	[[nodiscard]]
	checksum_type calculate_checksum(
		buffers::input_buffer_stateful_wrapper_ref& image_buffer) const;

private:
	checksum_type checksum_ = 0;
	std::size_t dans_relative_offset_ = 0;
	std::size_t dans_absolute_offset_ = 0;
	compid_list compids_;
};

} //namespace pe_bliss::rich

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::rich::rich_header_errc> : true_type {};
} //namespace std
