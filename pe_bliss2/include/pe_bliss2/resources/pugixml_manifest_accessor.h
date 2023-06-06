#pragma once

#include <memory>

#include "buffers/input_buffer_interface.h"
#include "pe_bliss2/error_list.h"
#include "pe_bliss2/resources/manifest_accessor_interface.h"

namespace pe_bliss::resources::pugixml
{

namespace impl
{
struct pugixml_manifest_accessor_impl;
} //namespace impl

class [[nodiscard]] pugixml_manifest_accessor final : public manifest_accessor_interface
{
public:
	explicit pugixml_manifest_accessor(const buffers::input_buffer_ptr& buffer);

public:
	[[nodiscard]]
	virtual const error_list& get_errors() const noexcept override
	{
		return errors_;
	}

	[[nodiscard]]
	virtual const manifest_node_interface* get_root() const override;

	[[nodiscard]]
	error_list& get_errors() noexcept
	{
		return errors_;
	}

private:
	error_list errors_;
	std::unique_ptr<impl::pugixml_manifest_accessor_impl> impl_;
};

[[nodiscard]]
manifest_accessor_interface_ptr parse_manifest(
	const buffers::input_buffer_ptr& buffer);

} //namespace pe_bliss::resources::pugixml
