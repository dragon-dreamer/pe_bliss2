#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "buffers/ref_buffer.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/detail/resources/version_info.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/packed_c_string.h"

namespace pe_bliss::resources
{

enum class version_info_value_type : std::uint16_t
{
	binary = detail::resources::version_info_block_value_type_binary,
	text = detail::resources::version_info_block_value_type_text
};

template<typename... Bases>
class [[nodiscard]] version_info_block_base
	: public detail::packed_struct_base<detail::resources::version_info_block>
	, public Bases...
{
public:
	using key_type = std::optional<packed_utf16_c_string>;
	using child_list_type = std::vector<version_info_block_base<Bases...>>;
	using value_type = std::variant<std::monostate,
		buffers::ref_buffer, packed_utf16_c_string>;

public:
	[[nodiscard]]
	key_type& get_key() & noexcept
	{
		return key_;
	}

	[[nodiscard]]
	const key_type& get_key() const& noexcept
	{
		return key_;
	}

	[[nodiscard]]
	key_type get_key() && noexcept
	{
		return std::move(key_);
	}

	[[nodiscard]]
	version_info_value_type get_value_type() const noexcept
	{
		return static_cast<version_info_value_type>(descriptor_->type);
	}

	[[nodiscard]]
	value_type& get_value() & noexcept
	{
		return value_;
	}

	[[nodiscard]]
	const value_type& get_value() const& noexcept
	{
		return value_;
	}

	[[nodiscard]]
	value_type get_value() && noexcept
	{
		return std::move(value_);
	}

	[[nodiscard]]
	child_list_type& get_children() & noexcept
	{
		return children_;
	}

	[[nodiscard]]
	const child_list_type& get_children() const& noexcept
	{
		return children_;
	}

	[[nodiscard]]
	child_list_type get_children() && noexcept
	{
		return std::move(children_);
	}

	[[nodiscard]]
	bool empty() const noexcept
	{
		return get_descriptor()->value_length == 0u;
	}

private:
	key_type key_;
	value_type value_;
	child_list_type children_;
};

using version_info_block = version_info_block_base<>;
using version_info_block_details = version_info_block_base<error_list>;

} //namespace pe_bliss::resources
