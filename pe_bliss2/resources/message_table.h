#pragma once

#include <cstdint>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>
#include <utility>

#include "pe_bliss2/packed_struct.h"
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
class [[nodiscard]] message_entry_base : public Bases...
{
public:
	using descriptor_type = packed_struct<
		detail::resources::message_resource_entry>;
	using message_type = std::variant<std::monostate,
		ansi_message, unicode_message, utf8_message>;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

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
	descriptor_type descriptor_;
	message_type message_;
};

template<typename... Bases>
class [[nodiscard]] message_block_base : public Bases...
{
public:
	using descriptor_type = packed_struct<
		detail::resources::message_resource_block>;
	using id_type = std::uint32_t;
	using message_entry_list_type = std::vector<
		message_entry_base<Bases...>>;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]] id_type get_start_id() const noexcept
	{
		return descriptor_->low_id;
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
	descriptor_type descriptor_;
	message_entry_list_type entries_;
};

template<typename... Bases>
class [[nodiscard]] message_table_base : public Bases...
{
public:
	using descriptor_type = packed_struct<detail::resources::message_resource_data>;
	using message_block_list_type = std::vector<message_block_base<Bases...>>;

public:
	[[nodiscard]]
	descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	const descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

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
	descriptor_type descriptor_;
	message_block_list_type message_blocks_;
};

using message_block = message_block_base<>;
using message_block_details = message_block_base<error_list>;
using message_table = message_table_base<>;
using message_table_details = message_table_base<error_list>;
using message_entry = message_entry_base<>;
using message_entry_details = message_entry_base<error_list>;

} //namespace pe_bliss::resources
