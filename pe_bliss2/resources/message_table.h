#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <utility>

#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/detail/resources/message_table.h"
#include "pe_bliss2/error_list.h"

namespace pe_bliss::resources
{

enum class message_encoding
{
	ansi = detail::resources::message_resource_ansi,
	unicode = detail::resources::message_resource_unicode,
	utf8 = detail::resources::message_resource_utf8
};

enum class message_severity : std::uint32_t
{
	success = detail::resources::error_severity_success,
	informational = detail::resources::error_severity_informational,
	warning = detail::resources::error_severity_warning,
	error = detail::resources::error_severity_error
};

[[nodiscard]] constexpr message_severity get_message_severity(
	std::uint32_t message_id) noexcept
{
	return static_cast<message_severity>(
		message_id & detail::resources::severity_mask);
}

[[nodiscard]] constexpr std::uint32_t set_message_severity(
	std::uint32_t message_id, message_severity severity) noexcept
{
	message_id &= ~detail::resources::severity_mask;
	message_id |= static_cast<std::uint32_t>(severity);
	return message_id;
}

[[nodiscard]] constexpr std::uint16_t get_message_facility(
	std::uint32_t message_id) noexcept
{
	return static_cast<std::uint16_t>(
		(message_id & detail::resources::facility_mask) >> 16u);
}

[[nodiscard]] constexpr std::uint32_t set_message_facility(
	std::uint32_t message_id, std::uint16_t facility) noexcept
{
	message_id &= ~detail::resources::facility_mask;
	message_id |= (static_cast<std::uint32_t>(facility) << 16u)
		& detail::resources::facility_mask;
	return message_id;
}

template<message_encoding Encoding>
class [[nodiscard]] message_string
{
public:
	static constexpr message_encoding encoding = Encoding;
	using string_type = std::conditional_t<
		encoding == message_encoding::unicode,
		std::u16string,
		std::conditional_t<
			encoding == message_encoding::utf8,
			std::u8string,
			std::string
		>
	>;

public:
	[[nodiscard]]
	string_type& value() & noexcept
	{
		return string_;
	}

	[[nodiscard]]
	const string_type& value() const& noexcept
	{
		return string_;
	}

	[[nodiscard]]
	string_type value() && noexcept
	{
		return std::move(string_);
	}

private:
	string_type string_;
};

using ansi_message = message_string<message_encoding::ansi>;
using unicode_message = message_string<message_encoding::unicode>;
using utf8_message = message_string<message_encoding::utf8>;

template<typename... Bases>
class [[nodiscard]] message_entry_base
	: public detail::packed_struct_base<detail::resources::message_resource_entry>
	, public Bases...
{
public:
	using message_type = std::variant<std::monostate,
		ansi_message, unicode_message, utf8_message>;

public:
	[[nodiscard]]
	const message_type& get_message() const& noexcept
	{
		return message_;
	}

	[[nodiscard]]
	message_type& get_message() & noexcept
	{
		return message_;
	}

	[[nodiscard]]
	message_type get_message() && noexcept
	{
		return std::move(message_);
	}

private:
	message_type message_;
};

template<typename... Bases>
class [[nodiscard]] message_block_base
	: public detail::packed_struct_base<detail::resources::message_resource_block>
	, public Bases...
{
public:
	using id_type = std::uint32_t;
	using message_entry_list_type = std::vector<
		message_entry_base<Bases...>>;

public:
	[[nodiscard]] id_type get_start_id() const noexcept
	{
		return this->descriptor_->low_id;
	}

	void set_start_id(id_type start_id) noexcept;
	
	[[nodiscard]]
	message_entry_list_type& get_entries() & noexcept
	{
		return entries_;
	}

	[[nodiscard]]
	const message_entry_list_type& get_entries() const& noexcept
	{
		return entries_;
	}

	[[nodiscard]]
	message_entry_list_type get_entries() && noexcept
	{
		return std::move(entries_);
	}

private:
	message_entry_list_type entries_;
};

template<typename... Bases>
class [[nodiscard]] message_table_base
	: public detail::packed_struct_base<detail::resources::message_resource_data>
	, public Bases...
{
public:
	using message_block_list_type = std::vector<message_block_base<Bases...>>;

public:
	[[nodiscard]]
	message_block_list_type& get_message_blocks() & noexcept
	{
		return message_blocks_;
	}

	[[nodiscard]]
	const message_block_list_type& get_message_blocks() const& noexcept
	{
		return message_blocks_;
	}

	[[nodiscard]]
	message_block_list_type get_message_blocks() && noexcept
	{
		return std::move(message_blocks_);
	}

private:
	message_block_list_type message_blocks_;
};

using message_block = message_block_base<>;
using message_block_details = message_block_base<error_list>;
using message_table = message_table_base<>;
using message_table_details = message_table_base<error_list>;
using message_entry = message_entry_base<>;
using message_entry_details = message_entry_base<error_list>;

} //namespace pe_bliss::resources
