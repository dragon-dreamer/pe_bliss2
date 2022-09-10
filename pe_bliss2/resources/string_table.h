#pragma once

#include <array>
#include <cstddef>
#include <utility>

#include "pe_bliss2/packed_utf16_string.h"

namespace pe_bliss::resources
{

class string_table
{
public:
	static constexpr std::uint16_t max_string_count = 16u;
	using string_list_type = std::array<packed_utf16_string, max_string_count>;
	using id_type = std::uint16_t;

public:
	[[nodiscard]]
	static id_type string_to_table_id(id_type string_id) noexcept
	{
		return (string_id >> 4u) + 1u;
	}

	[[nodiscard]]
	static id_type table_to_string_id(id_type table_id,
		std::uint8_t index) noexcept
	{
		return ((table_id - 1u) << 4u) + index;
	}

public:
	[[nodiscard]] id_type get_id() const noexcept
	{
		return table_id_;
	}

	void set_id(id_type id) noexcept
	{
		table_id_ = id;
	}

	[[nodiscard]] string_list_type& get_list() & noexcept
	{
		return list_;
	}

	[[nodiscard]] const string_list_type& get_list() const& noexcept
	{
		return list_;
	}

	[[nodiscard]] string_list_type get_list() && noexcept
	{
		return std::move(list_);
	}

private:
	id_type table_id_{};
	string_list_type list_{};
};

} //namespace pe_bliss::resources
