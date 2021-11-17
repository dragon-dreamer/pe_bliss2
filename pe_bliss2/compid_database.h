#pragma once

#include <cstdint>

#include "pe_bliss2/rich_compid.h"

namespace pe_bliss
{

class compid_database
{
public:
	enum class tool_type
	{
		resource_file,
		exported_symbol,
		imported_symbol,
		linker,
		assembly,
		c_source,
		cpp_source,
		unknown
	};

	enum class product_type
	{
		visual_studio_1997_sp3,
		visual_studio_6,
		visual_studio_6_sp5,
		visual_studio_6_sp6,
		windows_xp_sp1_ddk,
		visual_studio_2002,
		visual_studio_2003,
		windows_server_2003_sp1_ddk,
		visual_studio_2003_sp1,
		windows_server_2003_sp1_ddk_amd64,
		visual_studio_2005_beta1,
		visual_studio_2005_beta2,
		visual_studio_2005,
		visual_studio_2005_sp1,
		visual_studio_2008_beta2,
		visual_studio_2008,
		visual_studio_2008_sp1,
		visual_studio_2010_beta1,
		visual_studio_2010_beta2,
		visual_studio_2010,
		visual_studio_2010_sp1,
		visual_studio_2012,
		visual_studio_2012_november_ctp,
		visual_studio_2012_update1,
		visual_studio_2012_update2,
		visual_studio_2012_update3,
		visual_studio_2012_update4,
		visual_studio_2013_preview,
		visual_studio_2013_rc,
		visual_studio_2013_rtm,
		visual_studio_2013_update1 = visual_studio_2013_rtm,
		visual_studio_2013_november_ctp,
		visual_studio_2013_update2_rc,
		visual_studio_2013_update2,
		visual_studio_2013_update3,
		visual_studio_2013_update4,
		visual_studio_2013_update5,
		visual_studio_2015,
		visual_studio_2015_update1,
		visual_studio_2015_update2,
		visual_studio_2015_update3,
		visual_studio_2015_update3_1,
		visual_studio_2017_15_0,
		visual_studio_2017_15_3,
		visual_studio_2017_15_3_3,
		visual_studio_2017_15_4_4,
		visual_studio_2017_15_4_5,
		visual_studio_2017_15_5_2,
		visual_studio_2017_15_5_3,
		visual_studio_2017_15_5_6,
		visual_studio_2017_15_6_0,
		visual_studio_2017_15_6_3,
		visual_studio_2017_15_6_6,
		visual_studio_2017_15_6_7,
		visual_studio_2017_15_7_1,
		visual_studio_2017_15_7_2,
		visual_studio_2017_15_7_3,
		visual_studio_2017_15_7_4,
		visual_studio_2017_15_7_5,
		visual_studio_2017_15_8_0,
		visual_studio_2017_15_8_4,
		visual_studio_2017_15_8_9,
		visual_studio_2017_15_8_5,
		visual_studio_2017_15_9_1,
		visual_studio_2017_15_9_4,
		visual_studio_2017_15_9_5,
		visual_studio_2017_15_9_7,
		visual_studio_2017_15_9_11,
		visual_studio_2017_15_9_30,
		visual_studio_2019_16_0_0,
		visual_studio_2019_16_1_2,
		visual_studio_2019_16_2_3,
		visual_studio_2019_16_3_2,
		visual_studio_2019_16_4_0,
		visual_studio_2019_16_4_3,
		visual_studio_2019_16_4_4,
		visual_studio_2019_16_4_6,
		visual_studio_2019_16_5_0,
		visual_studio_2019_16_5_1,
		visual_studio_2019_16_5_2,
		visual_studio_2019_16_5_4,
		visual_studio_2019_16_6_0,
		visual_studio_2019_16_6_2,
		visual_studio_2019_16_7_0,
		visual_studio_2019_16_7_1,
		visual_studio_2019_16_7_5,
		visual_studio_2019_16_8_0,
		visual_studio_2019_16_8_2,
		visual_studio_2019_16_8_3,
		visual_studio_2019_16_8_4,
		visual_studio_2019_16_8_5,
		visual_studio_2019_16_9_0,
		visual_studio_2019_16_9_2,
		visual_studio_2019_16_9_4,
		visual_studio_2019_16_9_5,
		visual_studio_2019_16_10_0,
		visual_studio_2019_16_10_3,
		visual_studio_2019_16_10_4,
		visual_studio_2019_16_11_1,
		visual_studio_2019_16_11_5,
		visual_studio_2019_16_11_6,
		visual_studio_2022_17_0_0_preview2,
		visual_studio_2022_17_0_0_preview3_1,
		visual_studio_2022_17_0_0_preview4_0,
		visual_studio_2022_17_0_0_preview5_0,
		visual_studio_2022_17_0_0_preview7_0,
		visual_studio_2022_17_1_0_preview1_0,
		unmarked_object,
		unknown
	};

	struct product_type_info
	{
		product_type type = product_type::unknown;
		bool exact = true;
	};

	[[nodiscard]]
	static product_type_info get_product(const rich_compid& compid) noexcept;

	[[nodiscard]]
	static tool_type get_tool(std::uint16_t prod_id) noexcept;

	[[nodiscard]]
	static const char* product_type_to_string(product_type type) noexcept;

	[[nodiscard]]
	static const char* tool_type_to_string(tool_type type) noexcept;
};

} //namespace pe_bliss
