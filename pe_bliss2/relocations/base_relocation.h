#pragma once

#include <cstdint>
#include <list>

#include "pe_bliss2/detail/error_list.h"
#include "pe_bliss2/detail/packed_struct.h"
#include "pe_bliss2/detail/relocations/image_base_relocation.h"
#include "pe_bliss2/relocations/relocation_entry.h"

namespace pe_bliss::relocations
{

template<typename RelocationEntry>
class base_relocation_base
{
public:
	using entry_list_type = std::list<RelocationEntry>;
	using packed_descriptor_type = detail::packed_struct<detail::relocations::image_base_relocation>;

public:
	[[nodiscard]]
	const entry_list_type& get_relocations() const noexcept
	{
		return relocations_;
	}

	[[nodiscard]]
	entry_list_type& get_relocations() noexcept
	{
		return relocations_;
	}

	[[nodiscard]]
	const packed_descriptor_type& get_descriptor() const noexcept
	{
		return descriptor_;
	}

	[[nodiscard]]
	packed_descriptor_type& get_descriptor() noexcept
	{
		return descriptor_;
	}

private:
	packed_descriptor_type descriptor_;
	entry_list_type relocations_;
};

using base_relocation = base_relocation_base<relocation_entry>;

class base_relocation_details
	: public base_relocation_base<relocation_entry_details>
	, public detail::error_list
{
};

using base_relocation_list = std::list<base_relocation>;
using base_relocation_details_list = std::list<base_relocation_details>;

} //namespace pe_bliss::relocations
