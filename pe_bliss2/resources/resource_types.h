#pragma once

#include <cstdint>
#include <vector>

#include "pe_bliss2/packed_utf16_string.h"
#include "pe_bliss2/resources/lcid.h"

namespace pe_bliss::resources
{

enum class resource_type : std::uint32_t
{
	cursor = 1,
	bitmap = 2,
	icon = 3,
	menu = 4,
	dialog = 5,
	string = 6,
	fontdir = 7,
	font = 8,
	accelerator = 9,
	rcdata = 10,
	message_table = 11,
	cursor_group = 12,
	icon_group = 14,
	version = 16,
	dlginclude = 17,
	plugplay = 19,
	vxd = 20,
	anicursor = 21,
	aniicon = 22,
	html = 23,
	manifest = 24
};

enum class manifest_resource_id : std::uint32_t
{
	createprocess = 1,
	isolationaware = 2,
	isolationaware_nostaticimport = 3,
	isolationpolicy = 4,
	isolationpolicy_browser = 5
};

using resource_type_list_type = std::vector<resource_type>;
using resource_id_type = std::uint32_t;
using resource_id_list_type = std::vector<resource_id_type>;
using resource_name_type = packed_utf16_string;
using resource_name_list_type = std::vector<resource_name_type>;
using resource_language_list_type = std::vector<lcid_type>;

} //namespace pe_bliss::resources
