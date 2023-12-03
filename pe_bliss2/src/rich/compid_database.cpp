#include "pe_bliss2/rich/compid_database.h"

#include <algorithm>
#include <array>

namespace
{

using product_type = pe_bliss::rich::compid_database::product_type;

struct product_mapping
{
	std::uint16_t prod_id;
	std::uint16_t build_number;
	product_type type;

	friend constexpr bool operator<(const product_mapping& l, const product_mapping& r) noexcept
	{
		return l.prod_id < r.prod_id
			|| (l.prod_id == r.prod_id && l.build_number < r.build_number);
	}
};

consteval auto merge_with_newer_products()
{
	constexpr std::array older_products
	{
		product_mapping{ 0x00, 0000, product_type::unmarked_object },
		product_mapping{ 0x01, 0000, product_type::unmarked_object },
		product_mapping{ 0x02, 7303, product_type::visual_studio_1997_sp3 },
		product_mapping{ 0x04, 8168, product_type::visual_studio_6 },
		product_mapping{ 0x04, 8447, product_type::visual_studio_6_sp5 },
		product_mapping{ 0x06, 1668, product_type::visual_studio_1997_sp3 },
		product_mapping{ 0x06, 1720, product_type::visual_studio_6 },
		product_mapping{ 0x06, 1736, product_type::visual_studio_6_sp5 },
		product_mapping{ 0x0a, 8168, product_type::visual_studio_6 },
		product_mapping{ 0x0a, 8804, product_type::visual_studio_6_sp6 },
		product_mapping{ 0x0b, 8168, product_type::visual_studio_6 },
		product_mapping{ 0x0b, 8804, product_type::visual_studio_6_sp6 },
		product_mapping{ 0x0f, 3077, product_type::visual_studio_2003 },
		product_mapping{ 0x0f, 4035, product_type::windows_server_2003_sp1_ddk },
		product_mapping{ 0x0f, 6030, product_type::visual_studio_2003_sp1 },
		product_mapping{ 0x15, 8804, product_type::visual_studio_6_sp5 },
		product_mapping{ 0x16, 8804, product_type::visual_studio_6_sp5 },
		product_mapping{ 0x19, 9176, product_type::windows_xp_sp1_ddk },
		product_mapping{ 0x19, 9466, product_type::visual_studio_2002 },
		product_mapping{ 0x1c, 9176, product_type::windows_xp_sp1_ddk },
		product_mapping{ 0x1c, 9466, product_type::visual_studio_2002 },
		product_mapping{ 0x1d, 9176, product_type::windows_xp_sp1_ddk },
		product_mapping{ 0x1d, 9466, product_type::visual_studio_2002 },
		product_mapping{ 0x3d, 9176, product_type::windows_xp_sp1_ddk },
		product_mapping{ 0x3d, 9466, product_type::visual_studio_2002 },
		product_mapping{ 0x3f, 9176, product_type::windows_xp_sp1_ddk },
		product_mapping{ 0x3f, 9466, product_type::visual_studio_2002 },
		product_mapping{ 0x40, 9176, product_type::windows_xp_sp1_ddk },
		product_mapping{ 0x40, 9466, product_type::visual_studio_2002 },
		product_mapping{ 0x45, 9176, product_type::windows_xp_sp1_ddk },
		product_mapping{ 0x45, 9466, product_type::visual_studio_2002 },
		product_mapping{ 0x5a, 3077, product_type::visual_studio_2003 },
		product_mapping{ 0x5a, 4035, product_type::windows_server_2003_sp1_ddk },
		product_mapping{ 0x5a, 6030, product_type::visual_studio_2003_sp1 },
		product_mapping{ 0x5c, 3077, product_type::visual_studio_2003 },
		product_mapping{ 0x5c, 4035, product_type::windows_server_2003_sp1_ddk },
		product_mapping{ 0x5c, 6030, product_type::visual_studio_2003_sp1 },
		product_mapping{ 0x5d, 3077, product_type::visual_studio_2003 },
		product_mapping{ 0x5d, 4035, product_type::windows_server_2003_sp1_ddk },
		product_mapping{ 0x5d, 6030, product_type::visual_studio_2003_sp1 },
		product_mapping{ 0x5e, 3052, product_type::visual_studio_2003 },
		product_mapping{ 0x5e, 3077, product_type::visual_studio_2003 },
		product_mapping{ 0x5e, 4035, product_type::windows_server_2003_sp1_ddk },
		product_mapping{ 0x5e, 6030, product_type::visual_studio_2003_sp1 },
		product_mapping{ 0x5f, 3077, product_type::visual_studio_2003 },
		product_mapping{ 0x5f, 4035, product_type::windows_server_2003_sp1_ddk },
		product_mapping{ 0x5f, 6030, product_type::visual_studio_2003_sp1 },
		product_mapping{ 0x60, 3077, product_type::visual_studio_2003 },
		product_mapping{ 0x60, 4035, product_type::windows_server_2003_sp1_ddk },
		product_mapping{ 0x60, 6030, product_type::visual_studio_2003_sp1 },
		product_mapping{ 0x6d, 40310, product_type::windows_server_2003_sp1_ddk_amd64 },
		product_mapping{ 0x6d, 40607, product_type::visual_studio_2005_beta1 },
		product_mapping{ 0x6d, 50215, product_type::visual_studio_2005_beta2 },
		product_mapping{ 0x6d, 50320, product_type::visual_studio_2005 },
		product_mapping{ 0x6d, 50727, product_type::visual_studio_2005_sp1 },
		product_mapping{ 0x6e, 40310, product_type::windows_server_2003_sp1_ddk_amd64 },
		product_mapping{ 0x6e, 40607, product_type::visual_studio_2005_beta1 },
		product_mapping{ 0x6e, 50215, product_type::visual_studio_2005_beta2 },
		product_mapping{ 0x6e, 50320, product_type::visual_studio_2005 },
		product_mapping{ 0x6e, 50727, product_type::visual_studio_2005_sp1 },
		product_mapping{ 0x78, 40310, product_type::windows_server_2003_sp1_ddk_amd64 },
		product_mapping{ 0x78, 40607, product_type::visual_studio_2005_beta1 },
		product_mapping{ 0x78, 50215, product_type::visual_studio_2005_beta2 },
		product_mapping{ 0x78, 50320, product_type::visual_studio_2005 },
		product_mapping{ 0x78, 50727, product_type::visual_studio_2005_sp1 },
		product_mapping{ 0x7a, 40310, product_type::windows_server_2003_sp1_ddk_amd64 },
		product_mapping{ 0x7a, 40607, product_type::visual_studio_2005_beta1 },
		product_mapping{ 0x7a, 50215, product_type::visual_studio_2005_beta2 },
		product_mapping{ 0x7a, 50320, product_type::visual_studio_2005 },
		product_mapping{ 0x7a, 50727, product_type::visual_studio_2005_sp1 },
		product_mapping{ 0x7b, 40310, product_type::windows_server_2003_sp1_ddk_amd64 },
		product_mapping{ 0x7b, 40607, product_type::visual_studio_2005_beta1 },
		product_mapping{ 0x7b, 50215, product_type::visual_studio_2005_beta2 },
		product_mapping{ 0x7b, 50320, product_type::visual_studio_2005 },
		product_mapping{ 0x7b, 50727, product_type::visual_studio_2005_sp1 },
		product_mapping{ 0x7c, 40310, product_type::windows_server_2003_sp1_ddk_amd64 },
		product_mapping{ 0x7c, 40607, product_type::visual_studio_2005_beta1 },
		product_mapping{ 0x7c, 50215, product_type::visual_studio_2005_beta2 },
		product_mapping{ 0x7c, 50320, product_type::visual_studio_2005 },
		product_mapping{ 0x7c, 50727, product_type::visual_studio_2005_sp1 },
		product_mapping{ 0x7d, 40310, product_type::windows_server_2003_sp1_ddk_amd64 },
		product_mapping{ 0x7d, 40607, product_type::visual_studio_2005_beta1 },
		product_mapping{ 0x7d, 50215, product_type::visual_studio_2005_beta2 },
		product_mapping{ 0x7d, 50320, product_type::visual_studio_2005 },
		product_mapping{ 0x7d, 50727, product_type::visual_studio_2005_sp1 },
		product_mapping{ 0x83, 20706, product_type::visual_studio_2008_beta2 },
		product_mapping{ 0x83, 21022, product_type::visual_studio_2008 },
		product_mapping{ 0x83, 30729, product_type::visual_studio_2008_sp1 },
		product_mapping{ 0x84, 20706, product_type::visual_studio_2008_beta2 },
		product_mapping{ 0x84, 21022, product_type::visual_studio_2008 },
		product_mapping{ 0x84, 30729, product_type::visual_studio_2008_sp1 },
		product_mapping{ 0x91, 20706, product_type::visual_studio_2008_beta2 },
		product_mapping{ 0x91, 21022, product_type::visual_studio_2008 },
		product_mapping{ 0x91, 30729, product_type::visual_studio_2008_sp1 },
		product_mapping{ 0x92, 20706, product_type::visual_studio_2008_beta2 },
		product_mapping{ 0x92, 21022, product_type::visual_studio_2008 },
		product_mapping{ 0x92, 30729, product_type::visual_studio_2008_sp1 },
		product_mapping{ 0x93, 20706, product_type::visual_studio_2008_beta2 },
		product_mapping{ 0x93, 21022, product_type::visual_studio_2008 },
		product_mapping{ 0x93, 30729, product_type::visual_studio_2008_sp1 },
		product_mapping{ 0x94, 20706, product_type::visual_studio_2008_beta2 },
		product_mapping{ 0x94, 21022, product_type::visual_studio_2008 },
		product_mapping{ 0x94, 30729, product_type::visual_studio_2008_sp1 },
		product_mapping{ 0x95, 20706, product_type::visual_studio_2008_beta2 },
		product_mapping{ 0x95, 21022, product_type::visual_studio_2008 },
		product_mapping{ 0x95, 30729, product_type::visual_studio_2008_sp1 },
		product_mapping{ 0x9a, 20506, product_type::visual_studio_2010_beta1 },
		product_mapping{ 0x9a, 21003, product_type::visual_studio_2010_beta2 },
		product_mapping{ 0x9a, 30319, product_type::visual_studio_2010 },
		product_mapping{ 0x9a, 40219, product_type::visual_studio_2010_sp1 },
		product_mapping{ 0x9b, 20506, product_type::visual_studio_2010_beta1 },
		product_mapping{ 0x9b, 21003, product_type::visual_studio_2010_beta2 },
		product_mapping{ 0x9b, 30319, product_type::visual_studio_2010 },
		product_mapping{ 0x9b, 40219, product_type::visual_studio_2010_sp1 },
		product_mapping{ 0x9c, 20506, product_type::visual_studio_2010_beta1 },
		product_mapping{ 0x9c, 21003, product_type::visual_studio_2010_beta2 },
		product_mapping{ 0x9c, 30319, product_type::visual_studio_2010 },
		product_mapping{ 0x9c, 40219, product_type::visual_studio_2010_sp1 },
		product_mapping{ 0x9d, 20506, product_type::visual_studio_2010_beta1 },
		product_mapping{ 0x9d, 21003, product_type::visual_studio_2010_beta2 },
		product_mapping{ 0x9d, 30319, product_type::visual_studio_2010 },
		product_mapping{ 0x9d, 40219, product_type::visual_studio_2010_sp1 },
		product_mapping{ 0x9e, 20506, product_type::visual_studio_2010_beta1 },
		product_mapping{ 0x9e, 21003, product_type::visual_studio_2010_beta2 },
		product_mapping{ 0x9e, 30319, product_type::visual_studio_2010 },
		product_mapping{ 0x9e, 40219, product_type::visual_studio_2010_sp1 },
		product_mapping{ 0xaa, 20506, product_type::visual_studio_2010_beta1 },
		product_mapping{ 0xaa, 21003, product_type::visual_studio_2010_beta2 },
		product_mapping{ 0xaa, 30319, product_type::visual_studio_2010 },
		product_mapping{ 0xaa, 40219, product_type::visual_studio_2010_sp1 },
		product_mapping{ 0xab, 20506, product_type::visual_studio_2010_beta1 },
		product_mapping{ 0xab, 21003, product_type::visual_studio_2010_beta2 },
		product_mapping{ 0xab, 30319, product_type::visual_studio_2010 },
		product_mapping{ 0xab, 40219, product_type::visual_studio_2010_sp1 },
		product_mapping{ 0xc9, 50727, product_type::visual_studio_2012 },
		product_mapping{ 0xc9, 51025, product_type::visual_studio_2012_november_ctp },
		product_mapping{ 0xc9, 51106, product_type::visual_studio_2012_update1 },
		product_mapping{ 0xc9, 60315, product_type::visual_studio_2012_update2 },
		product_mapping{ 0xc9, 60610, product_type::visual_studio_2012_update3 },
		product_mapping{ 0xc9, 61030, product_type::visual_studio_2012_update4 },
		product_mapping{ 0xca, 50727, product_type::visual_studio_2012 },
		product_mapping{ 0xca, 51025, product_type::visual_studio_2012_november_ctp },
		product_mapping{ 0xca, 51106, product_type::visual_studio_2012_update1 },
		product_mapping{ 0xca, 60315, product_type::visual_studio_2012_update2 },
		product_mapping{ 0xca, 60610, product_type::visual_studio_2012_update3 },
		product_mapping{ 0xca, 61030, product_type::visual_studio_2012_update4 },
		product_mapping{ 0xcb, 50727, product_type::visual_studio_2012 },
		product_mapping{ 0xcb, 51025, product_type::visual_studio_2012_november_ctp },
		product_mapping{ 0xcb, 51106, product_type::visual_studio_2012_update1 },
		product_mapping{ 0xcb, 60315, product_type::visual_studio_2012_update2 },
		product_mapping{ 0xcb, 60610, product_type::visual_studio_2012_update3 },
		product_mapping{ 0xcb, 61030, product_type::visual_studio_2012_update4 },
		product_mapping{ 0xcc, 50727, product_type::visual_studio_2012 },
		product_mapping{ 0xcc, 51025, product_type::visual_studio_2012_november_ctp },
		product_mapping{ 0xcc, 51106, product_type::visual_studio_2012_update1 },
		product_mapping{ 0xcc, 60315, product_type::visual_studio_2012_update2 },
		product_mapping{ 0xcc, 60610, product_type::visual_studio_2012_update3 },
		product_mapping{ 0xcc, 61030, product_type::visual_studio_2012_update4 },
		product_mapping{ 0xcd, 50727, product_type::visual_studio_2012 },
		product_mapping{ 0xcd, 51025, product_type::visual_studio_2012_november_ctp },
		product_mapping{ 0xcd, 51106, product_type::visual_studio_2012_update1 },
		product_mapping{ 0xcd, 60315, product_type::visual_studio_2012_update2 },
		product_mapping{ 0xcd, 60610, product_type::visual_studio_2012_update3 },
		product_mapping{ 0xcd, 61030, product_type::visual_studio_2012_update4 },
		product_mapping{ 0xce, 50727, product_type::visual_studio_2012 },
		product_mapping{ 0xce, 51025, product_type::visual_studio_2012_november_ctp },
		product_mapping{ 0xce, 51106, product_type::visual_studio_2012_update1 },
		product_mapping{ 0xce, 60315, product_type::visual_studio_2012_update2 },
		product_mapping{ 0xce, 60610, product_type::visual_studio_2012_update3 },
		product_mapping{ 0xce, 61030, product_type::visual_studio_2012_update4 },
		product_mapping{ 0xcf, 50727, product_type::visual_studio_2012 },
		product_mapping{ 0xcf, 51025, product_type::visual_studio_2012_november_ctp },
		product_mapping{ 0xcf, 51106, product_type::visual_studio_2012_update1 },
		product_mapping{ 0xcf, 60315, product_type::visual_studio_2012_update2 },
		product_mapping{ 0xcf, 60610, product_type::visual_studio_2012_update3 },
		product_mapping{ 0xcf, 61030, product_type::visual_studio_2012_update4 },
		product_mapping{ 0xdb, 20617, product_type::visual_studio_2013_preview },
		product_mapping{ 0xdb, 20827, product_type::visual_studio_2013_rc },
		product_mapping{ 0xdb, 21005, product_type::visual_studio_2013_rtm },
		product_mapping{ 0xdb, 21114, product_type::visual_studio_2013_november_ctp },
		product_mapping{ 0xdb, 30324, product_type::visual_studio_2013_update2_rc },
		product_mapping{ 0xdb, 30501, product_type::visual_studio_2013_update2 },
		product_mapping{ 0xdb, 30723, product_type::visual_studio_2013_update3 },
		product_mapping{ 0xdb, 31101, product_type::visual_studio_2013_update4 },
		product_mapping{ 0xdb, 40629, product_type::visual_studio_2013_update5 },
		product_mapping{ 0xdc, 20617, product_type::visual_studio_2013_preview },
		product_mapping{ 0xdc, 20827, product_type::visual_studio_2013_rc },
		product_mapping{ 0xdc, 21005, product_type::visual_studio_2013_rtm },
		product_mapping{ 0xdc, 21114, product_type::visual_studio_2013_november_ctp },
		product_mapping{ 0xdc, 30324, product_type::visual_studio_2013_update2_rc },
		product_mapping{ 0xdc, 30501, product_type::visual_studio_2013_update2 },
		product_mapping{ 0xdc, 30723, product_type::visual_studio_2013_update3 },
		product_mapping{ 0xdc, 31101, product_type::visual_studio_2013_update4 },
		product_mapping{ 0xdc, 40629, product_type::visual_studio_2013_update5 },
		product_mapping{ 0xdd, 20617, product_type::visual_studio_2013_preview },
		product_mapping{ 0xdd, 20827, product_type::visual_studio_2013_rc },
		product_mapping{ 0xdd, 21005, product_type::visual_studio_2013_rtm },
		product_mapping{ 0xdd, 21114, product_type::visual_studio_2013_november_ctp },
		product_mapping{ 0xdd, 30324, product_type::visual_studio_2013_update2_rc },
		product_mapping{ 0xdd, 30501, product_type::visual_studio_2013_update2 },
		product_mapping{ 0xdd, 30723, product_type::visual_studio_2013_update3 },
		product_mapping{ 0xdd, 31101, product_type::visual_studio_2013_update4 },
		product_mapping{ 0xdd, 40629, product_type::visual_studio_2013_update5 },
		product_mapping{ 0xde, 20617, product_type::visual_studio_2013_preview },
		product_mapping{ 0xde, 20827, product_type::visual_studio_2013_rc },
		product_mapping{ 0xde, 21005, product_type::visual_studio_2013_rtm },
		product_mapping{ 0xde, 21114, product_type::visual_studio_2013_november_ctp },
		product_mapping{ 0xde, 30324, product_type::visual_studio_2013_update2_rc },
		product_mapping{ 0xde, 30501, product_type::visual_studio_2013_update2 },
		product_mapping{ 0xde, 30723, product_type::visual_studio_2013_update3 },
		product_mapping{ 0xde, 31101, product_type::visual_studio_2013_update4 },
		product_mapping{ 0xde, 40629, product_type::visual_studio_2013_update5 },
		product_mapping{ 0xdf, 20617, product_type::visual_studio_2013_preview },
		product_mapping{ 0xdf, 20827, product_type::visual_studio_2013_rc },
		product_mapping{ 0xdf, 21005, product_type::visual_studio_2013_rtm },
		product_mapping{ 0xdf, 21114, product_type::visual_studio_2013_november_ctp },
		product_mapping{ 0xdf, 30324, product_type::visual_studio_2013_update2_rc },
		product_mapping{ 0xdf, 30501, product_type::visual_studio_2013_update2 },
		product_mapping{ 0xdf, 30723, product_type::visual_studio_2013_update3 },
		product_mapping{ 0xdf, 31101, product_type::visual_studio_2013_update4 },
		product_mapping{ 0xdf, 40629, product_type::visual_studio_2013_update5 },
		product_mapping{ 0xe0, 20617, product_type::visual_studio_2013_preview },
		product_mapping{ 0xe0, 20827, product_type::visual_studio_2013_rc },
		product_mapping{ 0xe0, 21005, product_type::visual_studio_2013_rtm },
		product_mapping{ 0xe0, 21114, product_type::visual_studio_2013_november_ctp },
		product_mapping{ 0xe0, 30324, product_type::visual_studio_2013_update2_rc },
		product_mapping{ 0xe0, 30501, product_type::visual_studio_2013_update2 },
		product_mapping{ 0xe0, 30723, product_type::visual_studio_2013_update3 },
		product_mapping{ 0xe0, 31101, product_type::visual_studio_2013_update4 },
		product_mapping{ 0xe0, 40629, product_type::visual_studio_2013_update5 },
		product_mapping{ 0xe1, 20617, product_type::visual_studio_2013_preview },
		product_mapping{ 0xe1, 20827, product_type::visual_studio_2013_rc },
		product_mapping{ 0xe1, 21005, product_type::visual_studio_2013_rtm },
		product_mapping{ 0xe1, 21114, product_type::visual_studio_2013_november_ctp },
		product_mapping{ 0xe1, 30324, product_type::visual_studio_2013_update2_rc },
		product_mapping{ 0xe1, 30501, product_type::visual_studio_2013_update2 },
		product_mapping{ 0xe1, 30723, product_type::visual_studio_2013_update3 },
		product_mapping{ 0xe1, 31101, product_type::visual_studio_2013_update4 },
		product_mapping{ 0xe1, 40629, product_type::visual_studio_2013_update5 },
		product_mapping{ 0xfe, 0, product_type::cvtpgd },
		product_mapping{ 0xfe, 1, product_type::cvtpgd },
	};

	constexpr auto tool_types = std::to_array<std::uint16_t>({
		0x0ff, 0x100, 0x101, 0x102, 0x103, 0x104, 0x105,
		0x106, 0x107, 0x108, 0x109, 0x10a, 0x10b, 0x10c, 0x10d, 0x10e
	});

	constexpr std::array newer_products {
		product_mapping{ 0, 23026, product_type::visual_studio_2015 },
		product_mapping{ 0, 23506, product_type::visual_studio_2015_update1 },
		product_mapping{ 0, 23918, product_type::visual_studio_2015_update2 },
		product_mapping{ 0, 24210, product_type::visual_studio_2015_update3 },
		product_mapping{ 0, 24213, product_type::visual_studio_2015_update3 },
		product_mapping{ 0, 24215, product_type::visual_studio_2015_update3_1 },
		product_mapping{ 0, 25017, product_type::visual_studio_2017_15_0 },
		product_mapping{ 0, 25506, product_type::visual_studio_2017_15_3 },
		product_mapping{ 0, 25507, product_type::visual_studio_2017_15_3_3 },
		product_mapping{ 0, 25542, product_type::visual_studio_2017_15_4_4 },
		product_mapping{ 0, 25547, product_type::visual_studio_2017_15_4_5 },
		product_mapping{ 0, 25831, product_type::visual_studio_2017_15_5_2 },
		product_mapping{ 0, 25834, product_type::visual_studio_2017_15_5_3 },
		product_mapping{ 0, 25835, product_type::visual_studio_2017_15_5_6 },
		product_mapping{ 0, 26128, product_type::visual_studio_2017_15_6_0 },
		product_mapping{ 0, 26129, product_type::visual_studio_2017_15_6_3 },
		product_mapping{ 0, 26131, product_type::visual_studio_2017_15_6_6 },
		product_mapping{ 0, 26132, product_type::visual_studio_2017_15_6_7 },
		product_mapping{ 0, 26428, product_type::visual_studio_2017_15_7_1 },
		product_mapping{ 0, 26429, product_type::visual_studio_2017_15_7_2 },
		product_mapping{ 0, 26430, product_type::visual_studio_2017_15_7_3 },
		product_mapping{ 0, 26431, product_type::visual_studio_2017_15_7_4 },
		product_mapping{ 0, 26433, product_type::visual_studio_2017_15_7_5 },
		product_mapping{ 0, 26726, product_type::visual_studio_2017_15_8_0 },
		product_mapping{ 0, 26729, product_type::visual_studio_2017_15_8_4 },
		product_mapping{ 0, 26730, product_type::visual_studio_2017_15_8_9 },
		product_mapping{ 0, 26732, product_type::visual_studio_2017_15_8_5 },
		product_mapping{ 0, 27023, product_type::visual_studio_2017_15_9_1 },
		product_mapping{ 0, 27025, product_type::visual_studio_2017_15_9_4 },
		product_mapping{ 0, 27026, product_type::visual_studio_2017_15_9_5 },
		product_mapping{ 0, 27027, product_type::visual_studio_2017_15_9_7 },
		product_mapping{ 0, 27030, product_type::visual_studio_2017_15_9_11 },
		product_mapping{ 0, 27045, product_type::visual_studio_2017_15_9_30 },
		product_mapping{ 0, 27508, product_type::visual_studio_2019_16_0_0 },
		product_mapping{ 0, 27702, product_type::visual_studio_2019_16_1_2 },
		product_mapping{ 0, 27905, product_type::visual_studio_2019_16_2_3 },
		product_mapping{ 0, 28105, product_type::visual_studio_2019_16_3_2 },
		product_mapping{ 0, 28314, product_type::visual_studio_2019_16_4_0 },
		product_mapping{ 0, 28315, product_type::visual_studio_2019_16_4_3 },
		product_mapping{ 0, 28316, product_type::visual_studio_2019_16_4_4 },
		product_mapping{ 0, 28319, product_type::visual_studio_2019_16_4_6 },
		product_mapping{ 0, 28610, product_type::visual_studio_2019_16_5_0 },
		product_mapping{ 0, 28611, product_type::visual_studio_2019_16_5_1 },
		product_mapping{ 0, 28612, product_type::visual_studio_2019_16_5_2 },
		product_mapping{ 0, 28614, product_type::visual_studio_2019_16_5_4 },
		product_mapping{ 0, 28805, product_type::visual_studio_2019_16_6_0 },
		product_mapping{ 0, 28806, product_type::visual_studio_2019_16_6_2 },
		product_mapping{ 0, 29110, product_type::visual_studio_2019_16_7_0 },
		product_mapping{ 0, 29111, product_type::visual_studio_2019_16_7_1 },
		product_mapping{ 0, 29112, product_type::visual_studio_2019_16_7_5 },
		product_mapping{ 0, 29333, product_type::visual_studio_2019_16_8_0 },
		product_mapping{ 0, 29334, product_type::visual_studio_2019_16_8_2 },
		product_mapping{ 0, 29335, product_type::visual_studio_2019_16_8_3 },
		product_mapping{ 0, 29336, product_type::visual_studio_2019_16_8_4 },
		product_mapping{ 0, 29337, product_type::visual_studio_2019_16_8_5 },
		product_mapping{ 0, 29910, product_type::visual_studio_2019_16_9_0 },
		product_mapping{ 0, 29913, product_type::visual_studio_2019_16_9_2 },
		product_mapping{ 0, 29914, product_type::visual_studio_2019_16_9_4 },
		product_mapping{ 0, 29915, product_type::visual_studio_2019_16_9_5 },
		product_mapping{ 0, 30037, product_type::visual_studio_2019_16_10_0 },
		product_mapping{ 0, 30038, product_type::visual_studio_2019_16_10_3 },
		product_mapping{ 0, 30040, product_type::visual_studio_2019_16_10_4 },
		product_mapping{ 0, 30133, product_type::visual_studio_2019_16_11_1 },
		product_mapping{ 0, 30136, product_type::visual_studio_2019_16_11_5 },
		product_mapping{ 0, 30137, product_type::visual_studio_2019_16_11_6 },
		product_mapping{ 0, 30138, product_type::visual_studio_2019_16_11_8 },
		product_mapping{ 0, 30139, product_type::visual_studio_2019_16_11_9 },
		product_mapping{ 0, 30140, product_type::visual_studio_2019_16_11_10 },
		product_mapping{ 0, 30141, product_type::visual_studio_2019_16_11_11 },
		product_mapping{ 0, 30142, product_type::visual_studio_2019_16_11_12 },
		product_mapping{ 0, 30143, product_type::visual_studio_2019_16_11_13 },
		product_mapping{ 0, 30144, product_type::visual_studio_2019_16_11_14 },
		product_mapping{ 0, 30145, product_type::visual_studio_2019_16_11_15 },
		product_mapping{ 0, 30146, product_type::visual_studio_2019_16_11_17 },
		product_mapping{ 0, 30147, product_type::visual_studio_2019_16_11_21 },
		product_mapping{ 0, 30148, product_type::visual_studio_2019_16_11_24 },
		product_mapping{ 0, 30151, product_type::visual_studio_2019_16_11_27 },
		product_mapping{ 0, 30152, product_type::visual_studio_2019_16_11_30 },
		product_mapping{ 0, 30153, product_type::visual_studio_2019_16_11_32 },
		product_mapping{ 0, 30401, product_type::visual_studio_2022_17_0_0_preview2 },
		product_mapping{ 0, 30423, product_type::visual_studio_2022_17_0_0_preview3_1 },
		product_mapping{ 0, 30528, product_type::visual_studio_2022_17_0_0_preview4_0 },
		product_mapping{ 0, 30704, product_type::visual_studio_2022_17_0_0_preview5_0 },
		product_mapping{ 0, 30705, product_type::visual_studio_2022_17_0_0_preview7_0 },
		product_mapping{ 0, 30712, product_type::visual_studio_2022_17_0_21 },
		product_mapping{ 0, 30818, product_type::visual_studio_2022_17_1_0_preview1_0 },
		product_mapping{ 0, 30919, product_type::visual_studio_2022_17_1_0_preview2_0 },
		product_mapping{ 0, 31103, product_type::visual_studio_2022_17_1_0_preview3_0 },
		product_mapping{ 0, 31104, product_type::visual_studio_2022_17_1_0_preview5_0 },
		product_mapping{ 0, 31114, product_type::visual_studio_2022_17_2_0_preview1_0 },
		product_mapping{ 0, 31302, product_type::visual_studio_2022_17_2_0_preview2_1 },
		product_mapping{ 0, 31326, product_type::visual_studio_2022_17_2_0_preview3_0 },
		product_mapping{ 0, 31328, product_type::visual_studio_2022_17_2_0_preview5_0 },
		product_mapping{ 0, 31329, product_type::visual_studio_2022_17_2_1 },
		product_mapping{ 0, 31332, product_type::visual_studio_2022_17_2_5 },
		product_mapping{ 0, 31336, product_type::visual_studio_2022_17_2_15 },
		product_mapping{ 0, 31424, product_type::visual_studio_2022_17_3_0_preview1_0 },
		product_mapping{ 0, 31517, product_type::visual_studio_2022_17_3_0_preview2_0 },
		product_mapping{ 0, 31627, product_type::visual_studio_2022_17_3_0_preview3_0 },
		product_mapping{ 0, 31628, product_type::visual_studio_2022_17_3_0_preview4_0 },
		product_mapping{ 0, 31629, product_type::visual_studio_2022_17_3_0 },
		product_mapping{ 0, 31630, product_type::visual_studio_2022_17_3_4 },
		product_mapping{ 0, 31721, product_type::visual_studio_2022_17_4_0_preview1_0 },
		product_mapping{ 0, 31823, product_type::visual_studio_2022_17_4_0_preview2_0 },
		product_mapping{ 0, 31921, product_type::visual_studio_2022_17_4_0_preview3_0 },
		product_mapping{ 0, 31931, product_type::visual_studio_2022_17_4_0_preview4_0 },
		product_mapping{ 0, 31932, product_type::visual_studio_2022_17_4_0_preview5_0 },
		product_mapping{ 0, 31933, product_type::visual_studio_2022_17_4_0_preview6_0 },
		product_mapping{ 0, 31935, product_type::visual_studio_2022_17_4_2 },
		product_mapping{ 0, 31937, product_type::visual_studio_2022_17_4_3 },
		product_mapping{ 0, 31942, product_type::visual_studio_2022_17_4_5 },
		product_mapping{ 0, 32019, product_type::visual_studio_2022_17_5_0_preview1_0 },
		product_mapping{ 0, 32124, product_type::visual_studio_2022_17_5_0_preview2_0 },
		product_mapping{ 0, 32213, product_type::visual_studio_2022_17_5_0_preview4_0 },
		product_mapping{ 0, 32215, product_type::visual_studio_2022_17_5_0 },
		product_mapping{ 0, 32216, product_type::visual_studio_2022_17_5_3 },
		product_mapping{ 0, 32217, product_type::visual_studio_2022_17_5_4 },
		product_mapping{ 0, 32323, product_type::visual_studio_2022_17_6_0_preview1_0 },
		product_mapping{ 0, 32502, product_type::visual_studio_2022_17_6_0_preview2_0 },
		product_mapping{ 0, 32522, product_type::visual_studio_2022_17_6_0_preview3_0 },
		product_mapping{ 0, 32530, product_type::visual_studio_2022_17_6_0_preview6_0 },
		product_mapping{ 0, 32532, product_type::visual_studio_2022_17_6_0 },
		product_mapping{ 0, 32534, product_type::visual_studio_2022_17_6_3 },
		product_mapping{ 0, 32535, product_type::visual_studio_2022_17_6_4 },
		product_mapping{ 0, 32537, product_type::visual_studio_2022_17_6_5 },
		product_mapping{ 0, 32705, product_type::visual_studio_2022_17_7_0_preview1_0 },
		product_mapping{ 0, 32820, product_type::visual_studio_2022_17_7_0_preview3_0 },
		product_mapping{ 0, 32822, product_type::visual_studio_2022_17_7_0 },
		product_mapping{ 0, 32824, product_type::visual_studio_2022_17_7_4 },
		product_mapping{ 0, 32825, product_type::visual_studio_2022_17_7_5 },
		product_mapping{ 0, 32919, product_type::visual_studio_2022_17_8_0_preview1_0 },
		product_mapping{ 0, 33030, product_type::visual_studio_2022_17_8_0_preview2_0 },
		product_mapping{ 0, 33126, product_type::visual_studio_2022_17_8_0_preview3_0 },
		product_mapping{ 0, 33128, product_type::visual_studio_2022_17_8_0_preview5_0 },
		product_mapping{ 0, 33129, product_type::visual_studio_2022_17_8_0_preview6_0 },
		product_mapping{ 0, 33130, product_type::visual_studio_2022_17_8_0 },
		product_mapping{ 0, 33218, product_type::visual_studio_2022_17_9_0_preview1_0 },
	};

	std::array<product_mapping,
		tool_types.size() * newer_products.size() + older_products.size()> merged{};

	auto current = merged.begin();
	for (const auto& mapping : older_products)
		*current++ = mapping;

	for (auto tool : tool_types)
	{
		for (const auto& mapping : newer_products)
			*current++ = product_mapping{ tool, mapping.build_number, mapping.type };
	}

	return merged;
}

constexpr std::array products = merge_with_newer_products();

static_assert(std::is_sorted(products.cbegin(), products.cend()),
	"ProdID database must be sorted");

} //namespace

namespace pe_bliss::rich
{

compid_database::product_type_info compid_database::get_product(
	const rich_compid& compid) noexcept
{
	product_mapping target{ compid.prod_id, compid.build_number };
	auto [product_from, product_to] = std::equal_range(
		products.cbegin(), products.cend(), target,
		[] (const auto& l, const auto& r) {
			return l.prod_id < r.prod_id;
		});

	if (product_from == product_to)
		return {};

	auto it = std::lower_bound(product_from, product_to, target);
	if (it == product_to)
		return {};

	if (it->build_number > target.build_number && it != product_from)
		--it;

	return { .type = it->type, .exact = it->build_number == target.build_number };
}

compid_database::tool_type compid_database::get_tool(std::uint16_t prod_id) noexcept
{
	switch (prod_id)
	{
	case 0xff:
	case 0xdb:
	case 0xc9:
	case 0xa5:
	case 0x94:
	case 0x7c:
	case 0x5e:
	case 0x45:
	case 0x06:
	case 0x9a:
		return compid_database::tool_type::resource_file;
	case 0x100:
	case 0xdc:
	case 0xca:
	case 0xa6:
	case 0x92:
	case 0x7a:
	case 0x5c:
	case 0x3f:
	case 0x9b:
		return compid_database::tool_type::exported_symbol;
	case 0x101:
	case 0xdd:
	case 0xcb:
	case 0xa7:
	case 0x93:
	case 0x7b:
	case 0x5d:
	case 0x19:
	case 0x02:
	case 0x9c:
		return compid_database::tool_type::imported_symbol;
	case 0x102:
	case 0xde:
	case 0xcc:
	case 0xa8:
	case 0x91:
	case 0x78:
	case 0x5a:
	case 0x3d:
	case 0x04:
	case 0x9d:
		return compid_database::tool_type::linker;
	case 0x103:
	case 0xdf:
	case 0xcd:
	case 0xa9:
	case 0x95:
	case 0x7d:
	case 0x0f:
	case 0x40:
	case 0x9e:
		return compid_database::tool_type::assembly;
	case 0x104:
	case 0xe0:
	case 0xce:
	case 0xaa:
	case 0x83:
	case 0x6d:
	case 0x5f:
	case 0x1c:
	case 0x0a:
	case 0x15:
		return compid_database::tool_type::c_source;
	case 0x105:
	case 0xe1:
	case 0xcf:
	case 0xab:
	case 0x84:
	case 0x6e:
	case 0x60:
	case 0x1d:
	case 0x0b:
	case 0x16:
		return compid_database::tool_type::cpp_source;
	case 0x106:
		return compid_database::tool_type::cvtcil_c;
	case 0x107:
		return compid_database::tool_type::cvtcil_cpp;
	case 0x108:
		return compid_database::tool_type::link_time_code_generation_c;
	case 0x109:
		return compid_database::tool_type::link_time_code_generation_cpp;
	case 0x10a:
		return compid_database::tool_type::link_time_code_generation_msil;
	case 0x10b:
		return compid_database::tool_type::pogo_profiling_instrument_c;
	case 0x10c:
		return compid_database::tool_type::pogo_profiling_instrument_cpp;
	case 0x10d:
		return compid_database::tool_type::pogo_profiling_optimize_c;
	case 0x10e:
		return compid_database::tool_type::pogo_profiling_optimize_cpp;
	default:
		return compid_database::tool_type::unknown;
	}
}

std::string_view compid_database::product_type_to_string(product_type type) noexcept
{
	using enum compid_database::product_type;
	switch (type)
	{
	case visual_studio_1997_sp3: return "Visual Studio 1997 SP3";
	case visual_studio_6: return "Visual Studio 6";
	case visual_studio_6_sp5: return "Visual Studio 6 SP5";
	case visual_studio_6_sp6: return "Visual Studio 6 SP6";
	case windows_xp_sp1_ddk: return "WindowsXP SP1 DDK";
	case visual_studio_2002: return "Visual Studio 2002";
	case visual_studio_2003: return "Visual Studio 2003";
	case windows_server_2003_sp1_ddk: return "Windows Server 2003 SP1 DDK";
	case visual_studio_2003_sp1: return "Visual Studio 2003 SP1";
	case windows_server_2003_sp1_ddk_amd64: return "Windows Server 2003 SP1 DDK (AMD64)";
	case visual_studio_2005_beta1: return "Visual Studio 2005 Beta1";
	case visual_studio_2005_beta2: return "Visual Studio 2005 Beta2";
	case visual_studio_2005: return "Visual Studio 2005";
	case visual_studio_2005_sp1: return "Visual Studio 2005 SP1";
	case visual_studio_2008_beta2: return "Visual Studio 2008 Beta2";
	case visual_studio_2008: return "Visual Studio 2008";
	case visual_studio_2008_sp1: return "Visual Studio 2008 SP1";
	case visual_studio_2010_beta1: return "Visual Studio 2010 Beta1";
	case visual_studio_2010_beta2: return "Visual Studio 2010 Beta2";
	case visual_studio_2010: return "Visual Studio 2010";
	case visual_studio_2010_sp1: return "Visual Studio 2010 SP1";
	case visual_studio_2012: return "Visual Studio 2012";
	case visual_studio_2012_november_ctp: return "Visual Studio 2012 November CTP";
	case visual_studio_2012_update1: return "Visual Studio 2012 Update 1";
	case visual_studio_2012_update2: return "Visual Studio 2012 Update 2";
	case visual_studio_2012_update3: return "Visual Studio 2012 Update 3";
	case visual_studio_2012_update4: return "Visual Studio 2012 Update 4";
	case visual_studio_2013_preview: return "Visual Studio 2013 Preview";
	case visual_studio_2013_rc: return "Visual Studio 2013 RC";
	case visual_studio_2013_rtm: return "Visual Studio 2013 RTM/Update 1";
	case visual_studio_2013_november_ctp: return "Visual Studio 2013 November CTP";
	case visual_studio_2013_update2_rc: return "Visual Studio 2013 Update 2 RC";
	case visual_studio_2013_update2: return "Visual Studio 2013 Update 2";
	case visual_studio_2013_update3: return "Visual Studio 2013 Update 3";
	case visual_studio_2013_update4: return "Visual Studio 2013 Update 4";
	case visual_studio_2013_update5: return "Visual Studio 2013 Update 5";
	case visual_studio_2015: return "Visual Studio 2015";
	case visual_studio_2015_update1: return "Visual Studio 2015 Update 1";
	case visual_studio_2015_update2:return "Visual Studio 2015 Update 2";
	case visual_studio_2015_update3:return "Visual Studio 2015 Update 3";
	case visual_studio_2015_update3_1: return "Visual Studio 2015 Update 3.1";
	case visual_studio_2017_15_0: return "Visual Studio 2017 15.0";
	case visual_studio_2017_15_3: return "Visual Studio 2017 15.3";
	case visual_studio_2017_15_3_3: return "Visual Studio 2017 15.3.3";
	case visual_studio_2017_15_4_4: return "Visual Studio 2017 15.4.4";
	case visual_studio_2017_15_4_5: return "Visual Studio 2017 15.4.5";
	case visual_studio_2017_15_5_2: return "Visual Studio 2017 15.5.2";
	case visual_studio_2017_15_5_3: return "Visual Studio 2017 15.5.3";
	case visual_studio_2017_15_5_6: return "Visual Studio 2017 15.5.6";
	case visual_studio_2017_15_6_0: return "Visual Studio 2017 15.6.0";
	case visual_studio_2017_15_6_3: return "Visual Studio 2017 15.6.3";
	case visual_studio_2017_15_6_6: return "Visual Studio 2017 15.6.6";
	case visual_studio_2017_15_6_7: return "Visual Studio 2017 15.6.7";
	case visual_studio_2017_15_7_1: return "Visual Studio 2017 15.7.1";
	case visual_studio_2017_15_7_2: return "Visual Studio 2017 15.7.2";
	case visual_studio_2017_15_7_3: return "Visual Studio 2017 15.7.3";
	case visual_studio_2017_15_7_4: return "Visual Studio 2017 15.7.4";
	case visual_studio_2017_15_7_5: return "Visual Studio 2017 15.7.5";
	case visual_studio_2017_15_8_0: return "Visual Studio 2017 15.8.0";
	case visual_studio_2017_15_8_4: return "Visual Studio 2017 15.8.4";
	case visual_studio_2017_15_8_9: return "Visual Studio 2017 15.8.9";
	case visual_studio_2017_15_8_5: return "Visual Studio 2017 15.8.5";
	case visual_studio_2017_15_9_1: return "Visual Studio 2017 15.9.1";
	case visual_studio_2017_15_9_4: return "Visual Studio 2017 15.9.4";
	case visual_studio_2017_15_9_5: return "Visual Studio 2017 15.9.5";
	case visual_studio_2017_15_9_7: return "Visual Studio 2017 15.9.7";
	case visual_studio_2017_15_9_11: return "Visual Studio 2017 15.9.11";
	case visual_studio_2017_15_9_30: return "Visual Studio 2017 15.9.30";
	case visual_studio_2019_16_0_0: return "Visual Studio 2019 16.0.0";
	case visual_studio_2019_16_1_2: return "Visual Studio 2019 16.1.2";
	case visual_studio_2019_16_2_3: return "Visual Studio 2019 16.2.3";
	case visual_studio_2019_16_3_2: return "Visual Studio 2019 16.3.2";
	case visual_studio_2019_16_4_0: return "Visual Studio 2019 16.4.0";
	case visual_studio_2019_16_4_3: return "Visual Studio 2019 16.4.3";
	case visual_studio_2019_16_4_4: return "Visual Studio 2019 16.4.4";
	case visual_studio_2019_16_4_6: return "Visual Studio 2019 16.4.6";
	case visual_studio_2019_16_5_0: return "Visual Studio 2019 16.5.0";
	case visual_studio_2019_16_5_1: return "Visual Studio 2019 16.5.1";
	case visual_studio_2019_16_5_2: return "Visual Studio 2019 16.5.2";
	case visual_studio_2019_16_5_4: return "Visual Studio 2019 16.5.4";
	case visual_studio_2019_16_6_0: return "Visual Studio 2019 16.6.0";
	case visual_studio_2019_16_6_2: return "Visual Studio 2019 16.6.2";
	case visual_studio_2019_16_7_0: return "Visual Studio 2019 16.7.0";
	case visual_studio_2019_16_7_1: return "Visual Studio 2019 16.7.1";
	case visual_studio_2019_16_7_5: return "Visual Studio 2019 16.7.5";
	case visual_studio_2019_16_8_0: return "Visual Studio 2019 16.8.0";
	case visual_studio_2019_16_8_2: return "Visual Studio 2019 16.8.2";
	case visual_studio_2019_16_8_3: return "Visual Studio 2019 16.8.3";
	case visual_studio_2019_16_8_4: return "Visual Studio 2019 16.8.4";
	case visual_studio_2019_16_8_5: return "Visual Studio 2019 16.8.5";
	case visual_studio_2019_16_9_0: return "Visual Studio 2019 16.9.0";
	case visual_studio_2019_16_9_2: return "Visual Studio 2019 16.9.2";
	case visual_studio_2019_16_9_4: return "Visual Studio 2019 16.9.4";
	case visual_studio_2019_16_9_5: return "Visual Studio 2019 16.9.5";
	case visual_studio_2019_16_10_0: return "Visual Studio 2019 16.10.0";
	case visual_studio_2019_16_10_3: return "Visual Studio 2019 16.10.3";
	case visual_studio_2019_16_10_4: return "Visual Studio 2019 16.10.4";
	case visual_studio_2019_16_11_1: return "Visual Studio 2019 16.11.1";
	case visual_studio_2019_16_11_5: return "Visual Studio 2019 16.11.5";
	case visual_studio_2019_16_11_6: return "Visual Studio 2019 16.11.6";
	case visual_studio_2019_16_11_8: return "Visual Studio 2019 16.11.8";
	case visual_studio_2019_16_11_9: return "Visual Studio 2019 16.11.9";
	case visual_studio_2019_16_11_10: return "Visual Studio 2019 16.11.10";
	case visual_studio_2019_16_11_11: return "Visual Studio 2019 16.11.11";
	case visual_studio_2019_16_11_12: return "Visual Studio 2019 16.11.12";
	case visual_studio_2019_16_11_13: return "Visual Studio 2019 16.11.13";
	case visual_studio_2019_16_11_14: return "Visual Studio 2019 16.11.14";
	case visual_studio_2019_16_11_15: return "Visual Studio 2019 16.11.15";
	case visual_studio_2019_16_11_17: return "Visual Studio 2019 16.11.17";
	case visual_studio_2019_16_11_21: return "Visual Studio 2019 16.11.21";
	case visual_studio_2019_16_11_24: return "Visual Studio 2019 16.11.24";
	case visual_studio_2019_16_11_27: return "Visual Studio 2019 16.11.27";
	case visual_studio_2019_16_11_30: return "Visual Studio 2019 16.11.30";
	case visual_studio_2019_16_11_32: return "Visual Studio 2019 16.11.32";
	case visual_studio_2022_17_0_0_preview2: return "Visual Studio 2022 17.0.0 Preview 2";
	case visual_studio_2022_17_0_0_preview3_1: return "Visual Studio 2022 17.0.0 Preview 3.1";
	case visual_studio_2022_17_0_0_preview4_0: return "Visual Studio 2022 17.0.0 Preview 4.0";
	case visual_studio_2022_17_0_0_preview5_0: return "Visual Studio 2022 17.0.0 Preview 5.0";
	case visual_studio_2022_17_0_0_preview7_0: return "Visual Studio 2022 17.0.0 Preview 7.0";
	case visual_studio_2022_17_0_21: return "Visual Studio 2022 17.0.21";
	case visual_studio_2022_17_1_0_preview1_0: return "Visual Studio 2022 17.1.0 Preview 1.0";
	case visual_studio_2022_17_1_0_preview2_0: return "Visual Studio 2022 17.1.0 Preview 2.0";
	case visual_studio_2022_17_1_0_preview3_0: return "Visual Studio 2022 17.1.0 Preview 3.0";
	case visual_studio_2022_17_1_0_preview5_0: return "Visual Studio 2022 17.1.0 Preview 5.0";
	case visual_studio_2022_17_2_0_preview1_0: return "Visual Studio 2022 17.2.0 Preview 1.0";
	case visual_studio_2022_17_2_0_preview2_1: return "Visual Studio 2022 17.2.0 Preview 2.1";
	case visual_studio_2022_17_2_0_preview3_0: return "Visual Studio 2022 17.2.0 Preview 3.0";
	case visual_studio_2022_17_2_0_preview5_0: return "Visual Studio 2022 17.2.0 Preview 5.0";
	case visual_studio_2022_17_2_1: return "Visual Studio 2022 17.2.1";
	case visual_studio_2022_17_2_5: return "Visual Studio 2022 17.2.5";
	case visual_studio_2022_17_2_15: return "Visual Studio 2022 17.2.15";
	case visual_studio_2022_17_3_0_preview1_0: return "Visual Studio 2022 17.3.0 Preview 1.0";
	case visual_studio_2022_17_3_0_preview2_0: return "Visual Studio 2022 17.3.0 Preview 2.0";
	case visual_studio_2022_17_3_0_preview3_0: return "Visual Studio 2022 17.3.0 Preview 3.0";
	case visual_studio_2022_17_3_0_preview4_0: return "Visual Studio 2022 17.3.0 Preview 4.0";
	case visual_studio_2022_17_3_0: return "Visual Studio 2022 17.3.0";
	case visual_studio_2022_17_3_4: return "Visual Studio 2022 17.3.4";
	case visual_studio_2022_17_4_0_preview1_0: return "Visual Studio 2022 17.4.0 Preview 1.0";
	case visual_studio_2022_17_4_0_preview2_0: return "Visual Studio 2022 17.4.0 Preview 2.0";
	case visual_studio_2022_17_4_0_preview3_0: return "Visual Studio 2022 17.4.0 Preview 3.0";
	case visual_studio_2022_17_4_0_preview4_0: return "Visual Studio 2022 17.4.0 Preview 4.0";
	case visual_studio_2022_17_4_0_preview5_0: return "Visual Studio 2022 17.4.0 Preview 5.0";
	case visual_studio_2022_17_4_0_preview6_0: return "Visual Studio 2022 17.4.0 Preview 6.0";
	case visual_studio_2022_17_4_2: return "Visual Studio 2022 17.4.2";
	case visual_studio_2022_17_4_3: return "Visual Studio 2022 17.4.3";
	case visual_studio_2022_17_4_5: return "Visual Studio 2022 17.4.5";
	case visual_studio_2022_17_5_0_preview1_0: return "Visual Studio 2022 17.5.0 Preview 1.0";
	case visual_studio_2022_17_5_0_preview2_0: return "Visual Studio 2022 17.5.0 Preview 2.0";
	case visual_studio_2022_17_5_0_preview4_0: return "Visual Studio 2022 17.5.0 Preview 4.0";
	case visual_studio_2022_17_5_0: return "Visual Studio 2022 17.5.0";
	case visual_studio_2022_17_5_3: return "Visual Studio 2022 17.5.3";
	case visual_studio_2022_17_5_4: return "Visual Studio 2022 17.5.4";
	case visual_studio_2022_17_6_0_preview1_0: return "Visual Studio 2022 17.6.0 Preview 1.0";
	case visual_studio_2022_17_6_0_preview2_0: return "Visual Studio 2022 17.6.0 Preview 2.0";
	case visual_studio_2022_17_6_0_preview3_0: return "Visual Studio 2022 17.6.0 Preview 3.0";
	case visual_studio_2022_17_6_0_preview6_0: return "Visual Studio 2022 17.6.0 Preview 6.0";
	case visual_studio_2022_17_6_0: return "Visual Studio 2022 17.6.0";
	case visual_studio_2022_17_6_3: return "Visual Studio 2022 17.6.3";
	case visual_studio_2022_17_6_4: return "Visual Studio 2022 17.6.4";
	case visual_studio_2022_17_6_5: return "Visual Studio 2022 17.6.5";
	case visual_studio_2022_17_7_0_preview1_0: return "Visual Studio 2022 17.7.0 Preview 1.0";
	case visual_studio_2022_17_7_0_preview3_0: return "Visual Studio 2022 17.7.0 Preview 3.0";
	case visual_studio_2022_17_7_0: return "Visual Studio 2022 17.7.0";
	case visual_studio_2022_17_7_4: return "Visual Studio 2022 17.7.4";
	case visual_studio_2022_17_7_5: return "Visual Studio 2022 17.7.5";
	case visual_studio_2022_17_8_0_preview1_0: return "Visual Studio 2022 17.8.0 Preview 1.0";
	case visual_studio_2022_17_8_0_preview2_0: return "Visual Studio 2022 17.8.0 Preview 2.0";
	case visual_studio_2022_17_8_0_preview3_0: return "Visual Studio 2022 17.8.0 Preview 3.0";
	case visual_studio_2022_17_8_0_preview5_0: return "Visual Studio 2022 17.8.0 Preview 5.0";
	case visual_studio_2022_17_8_0_preview6_0: return "Visual Studio 2022 17.8.0 Preview 6.0";
	case visual_studio_2022_17_8_0: return "Visual Studio 2022 17.8.0";
	case visual_studio_2022_17_9_0_preview1_0: return "Visual Studio 2022 17.9.0 Preview 1.0";
	case unmarked_object: return "Unmarked object";
	case cvtpgd: return "CVTPGD";
	case cvtpgd_1: return "CVTPGD 1";
	default: return "Unknown";
	}
}

std::string_view compid_database::tool_type_to_string(tool_type type) noexcept
{
	using enum compid_database::tool_type;
	switch (type)
	{
	case resource_file: return "Resource";
	case exported_symbol: return "Export";
	case imported_symbol: return "Import";
	case linker: return "Linker";
	case assembly: return "Assembly";
	case c_source: return "C Source";
	case cpp_source: return "CPP Source";
	case link_time_code_generation_c: return "C Link Time Code Generation";
	case link_time_code_generation_cpp: return "CPP Link Time Code Generation";
	case link_time_code_generation_msil: return "MSIL Link Time Code Generation";
	case pogo_profiling_instrument_c: return "C POGO Profiling (Instrument)";
	case pogo_profiling_instrument_cpp: return "CPP POGO Profiling (Instrument)";
	case pogo_profiling_optimize_c: return "C POGO Profiling (Optimize)";
	case pogo_profiling_optimize_cpp: return "CPP POGO Profiling (Optimize)";
	case cvtcil_c: return "C CVTCIL";
	case cvtcil_cpp: return "CPP CVTCIL";
	default: return "Unknown";
	}
}

} //namespace pe_bliss::rich
