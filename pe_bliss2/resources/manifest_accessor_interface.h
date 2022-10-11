#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <system_error>

#include "pe_bliss2/error_list.h"

namespace pe_bliss::resources
{

enum class manifest_loader_errc
{
	invalid_xml = 1,
};

std::error_code make_error_code(manifest_loader_errc) noexcept;

class [[nodiscard]] manifest_named_entity_interface
{
public:
	virtual ~manifest_named_entity_interface() = default;

	[[nodiscard]]
	virtual std::string_view get_name() const = 0;

	[[nodiscard]]
	virtual std::string_view get_namespace() const = 0;

	[[nodiscard]]
	virtual std::string_view get_value() const = 0;
};

class manifest_node_interface;

class [[nodiscard]] manifest_node_iterator_interface
{
public:
	virtual ~manifest_node_iterator_interface() = default;

	[[nodiscard]]
	virtual const manifest_node_interface* first_child(
		std::string_view ns_name, std::string_view name) = 0;

	[[nodiscard]]
	virtual const manifest_node_interface* next_child() = 0;
};

using manifest_node_iterator_interface_ptr
	= std::unique_ptr<manifest_node_iterator_interface>;

class [[nodiscard]] manifest_node_interface
	: public manifest_named_entity_interface
{
public:
	[[nodiscard]]
	virtual std::size_t get_child_count() const = 0;

	[[nodiscard]]
	virtual std::size_t get_node_index() const noexcept = 0;

	[[nodiscard]]
	virtual manifest_node_iterator_interface_ptr get_iterator() const = 0;

	[[nodiscard]]
	virtual const manifest_named_entity_interface* get_attribute(
		std::string_view ns_name, std::string_view name) const = 0;
};

class [[nodiscard]] manifest_accessor_interface
{
public:
	virtual ~manifest_accessor_interface() = default;

	[[nodiscard]]
	virtual const error_list& get_errors() const noexcept = 0;

	[[nodiscard]]
	virtual const manifest_node_interface* get_root() const = 0;
};

using manifest_accessor_interface_ptr = std::shared_ptr<manifest_accessor_interface>;

} //namespace pe_bliss::resources

namespace std
{
template<>
struct is_error_code_enum<pe_bliss::resources::manifest_loader_errc> : true_type {};
} //namespace std
