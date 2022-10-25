#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "pe_bliss2/error_list.h"
#include "pe_bliss2/detail/packed_struct_base.h"
#include "pe_bliss2/detail/relocations/image_base_relocation.h"
#include "pe_bliss2/relocations/relocation_entry.h"

namespace pe_bliss::relocations
{

template<typename RelocationEntry>
class [[nodiscard]] base_relocation_base
	: public detail::packed_struct_base<detail::relocations::image_base_relocation>
{
public:
	using entry_list_type = std::vector<RelocationEntry>;

public:
	[[nodiscard]]
	const entry_list_type& get_relocations() const & noexcept
	{
		return relocations_;
	}

	[[nodiscard]]
	entry_list_type& get_relocations() & noexcept
	{
		return relocations_;
	}
	[[nodiscard]]
	entry_list_type get_relocations() && noexcept
	{
		return std::move(relocations_);
	}

private:
	entry_list_type relocations_;
};

using base_relocation = base_relocation_base<relocation_entry>;

class [[nodiscard]] base_relocation_details
	: public base_relocation_base<relocation_entry_details>
	, public error_list
{
};

using base_relocation_list = std::vector<base_relocation>;
using base_relocation_details_list = std::vector<base_relocation_details>;

} //namespace pe_bliss::relocations
