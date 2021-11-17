#pragma once

#include <cstdint>
#include <cstddef>
#include <list>
#include <system_error>
#include <type_traits>
#include <utility>

#include "pe_bliss2/dos_header.h"
#include "pe_bliss2/dos_stub.h"
#include "pe_bliss2/rich_compid.h"

namespace pe_bliss
{

enum class rich_header_errc
{
	invalid_rich_header_offset = 1
};

struct rich_header_serialization_options
{
	bool allow_dos_stub_buffer_extension = false;
	bool recalculate_checksum = true;
	std::size_t dos_stub_offset = 0;
};

std::error_code make_error_code(rich_header_errc) noexcept;

class rich_header
{
public:
	using compid_list = std::list<rich_compid>;

public:
	bool deserialize(const dos_stub::dos_stub_data_type& stub_data);
	std::byte* serialize(const dos_header& header, dos_stub::dos_stub_data_type& stub_data,
		const rich_header_serialization_options& options);

	[[nodiscard]] compid_list& get_compids() & noexcept
	{
		return compids_;
	}

	[[nodiscard]] const compid_list& get_compids() const & noexcept
	{
		return compids_;
	}
	
	[[nodiscard]] compid_list get_compids() && noexcept
	{
		auto result = std::move(compids_);
		compids_.clear();
		return std::move(result);
	}

	[[nodiscard]] std::uint32_t get_checksum() const noexcept
	{
		return checksum_;
	}

	[[nodiscard]] std::size_t dos_stub_offset() const noexcept
	{
		return dans_offset_;
	}

	void set_dos_stub_offset(std::size_t buffer_pos) noexcept
	{
		dans_offset_ = buffer_pos;
	}

	void set_checksum(std::uint32_t checksum) noexcept
	{
		checksum_ = checksum;
	}

	[[nodiscard]] std::uint32_t calculate_checksum(
		const dos_stub::dos_stub_data_type& stub_data, const dos_header& header,
		std::size_t rich_header_offset) const;

	[[nodiscard]] bool is_valid() const noexcept
	{
		return dans_offset_ && !compids_.empty();
	}

	[[nodiscard]] std::size_t calculate_size(std::size_t rich_header_offset) const;

private:
	std::size_t get_compid_offset() const noexcept;

	bool decode_compids(const std::vector<std::byte>& data,
		const std::byte* rich_signature_pointer);
	bool decode_dans_signature(const std::vector<std::byte>& data,
		const std::byte* compid_pointer) noexcept;
	std::size_t decode_checksum(const std::vector<std::byte>& data) noexcept;

private:
	static constexpr std::uint32_t dans_signature = 0x536E6144;
	static constexpr std::uint32_t compid_alignment = 16;
	static constexpr auto rich_compid_size = detail::packed_serialization<>::get_type_size<rich_compid>();

private:
	std::uint32_t checksum_ = 0;
	std::size_t dans_offset_ = 0;
	compid_list compids_;
};

} //namespace pe_bliss

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::rich_header_errc> : true_type {};
} //namespace std
