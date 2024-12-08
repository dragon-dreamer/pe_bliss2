#pragma once

#include <optional>

#include "pe_bliss2/trustlet/trustlet_policy_metadata.h"

namespace pe_bliss::image
{
class image;
} //namespace pe_bliss::image

namespace pe_bliss::trustlet
{

enum class trustlet_check_result
{
	valid_trustlet,
	absent_metadata,
	metadata_in_wrong_section,
	invalid_metadata_section_attributes
};

template<typename ExportDirectory>
[[nodiscard]]
trustlet_check_result is_trustlet(
	const image::image& instance,
	const ExportDirectory& export_dir);

template<typename ExportDirectory>
[[nodiscard]]
std::optional<trustlet_policy_metadata_details> load_trustlet_policy_metadata(
	const image::image& instance,
	const ExportDirectory& export_dir);

} //namespace pe_bliss::trustlet
