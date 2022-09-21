#include "pe_bliss2/resources/cpid.h"

namespace pe_bliss::resources
{

namespace
{
//See https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers,
//https://docs.microsoft.com/en-us/windows/win32/menurc/varfileinfo-block

std::string_view get_cpid_name_impl(code_page cpid) noexcept
{
	using enum code_page;
	switch (cpid)
	{
	case ibm037: return "IBM037";
	case ibm437: return "IBM437";
	case ibm500: return "IBM500";
	case asmo_708: return "ASMO-708";
	case arabic_asmo_449: return "Arabic(ASMO-449+, BCON V4)";
	case arabic_transparent_arabic: return "Arabic - Transparent Arabic";
	case dos_720: return "DOS-720";
	case ibm737: return "ibm737";
	case ibm775: return "ibm775";
	case ibm850: return "ibm850";
	case ibm852: return "ibm852";
	case ibm855: return "IBM855";
	case ibm857: return "ibm857";
	case ibm00858: return "IBM00858";
	case ibm860: return "IBM860";
	case ibm861: return "ibm861";
	case dos_862: return "DOS-862";
	case ibm863: return "IBM863";
	case ibm864: return "IBM864";
	case ibm865: return "IBM865";
	case cp866: return "cp866";
	case ibm869: return "ibm869";
	case ibm870: return "IBM870";
	case windows_874: return "windows-874";
	case cp875: return "cp875";
	case shift_jis: return "shift_jis";
	case gb2312: return "gb2312";
	case ks_c_5601_1987: return "ks_c_5601-1987";
	case big5: return "big5";
	case ibm1026: return "IBM1026";
	case ibm01047: return "IBM01047";
	case ibm01140: return "IBM01140";
	case ibm01141: return "IBM01141";
	case ibm01142: return "IBM01142";
	case ibm01143: return "IBM01143";
	case ibm01144: return "IBM01144";
	case ibm01145: return "IBM01145";
	case ibm01146: return "IBM01146";
	case ibm01147: return "IBM01147";
	case ibm01148: return "IBM01148";
	case ibm01149: return "IBM01149";
	case utf_16: return "utf-16";
	case unicodefffe: return "unicodeFFFE";
	case windows_1250: return "windows-1250";
	case windows_1251: return "windows-1251";
	case windows_1252: return "windows-1252";
	case windows_1253: return "windows-1253";
	case windows_1254: return "windows-1254";
	case windows_1255: return "windows-1255";
	case windows_1256: return "windows-1256";
	case windows_1257: return "windows-1257";
	case windows_1258: return "windows-1258";
	case johab: return "Johab";
	case macintosh: return "macintosh";
	case x_mac_japanese: return "x-mac-japanese";
	case x_mac_chinesetrad: return "x-mac-chinesetrad";
	case x_mac_korean: return "x-mac-korean";
	case x_mac_arabic: return "x-mac-arabic";
	case x_mac_hebrew: return "x-mac-hebrew";
	case x_mac_greek: return "x-mac-greek";
	case x_mac_cyrillic: return "x-mac-cyrillic";
	case x_mac_chinesesimp: return "x-mac-chinesesimp";
	case x_mac_romanian: return "x-mac-romanian";
	case x_mac_ukrainian: return "x-mac-ukrainian";
	case x_mac_thai: return "x-mac-thai";
	case x_mac_ce: return "x-mac-ce";
	case x_mac_icelandic: return "x-mac-icelandic";
	case x_mac_turkish: return "x-mac-turkish";
	case x_mac_croatian: return "x-mac-croatian";
	case utf_32: return "utf-32";
	case utf_32be: return "utf-32BE";
	case x_chinese_cns: return "x-Chinese_CNS";
	case x_cp20001: return "x-cp20001";
	case x_chinese_eten: return "x-Chinese-Eten";
	case x_cp20003: return "x-cp20003";
	case x_cp20004: return "x-cp20004";
	case x_cp20005: return "x-cp20005";
	case x_ia5: return "x-IA5";
	case x_ia5_german: return "x-IA5-German";
	case x_ia5_swedish: return "x-IA5-Swedish";
	case x_ia5_norwegian: return "x-IA5-Norwegian";
	case us_ascii: return "us-ascii";
	case x_cp20261: return "x-cp20261";
	case x_cp20269: return "x-cp20269";
	case ibm273: return "IBM273";
	case ibm277: return "IBM277";
	case ibm278: return "IBM278";
	case ibm280: return "IBM280";
	case ibm284: return "IBM284";
	case ibm285: return "IBM285";
	case ibm290: return "IBM290";
	case ibm297: return "IBM297";
	case ibm420: return "IBM420";
	case ibm423: return "IBM423";
	case ibm424: return "IBM424";
	case x_ebcdic_koreanextended: return "x-EBCDIC-KoreanExtended";
	case ibm_thai: return "IBM-Thai";
	case koi8_r: return "koi8-r";
	case ibm871: return "IBM871";
	case ibm880: return "IBM880";
	case ibm905: return "IBM905";
	case ibm00924: return "IBM00924";
	case euc_jp_jis_0208_1990_0212_1990: return "EUC-JP";
	case x_cp20936: return "x-cp20936";
	case x_cp20949: return "x-cp20949";
	case cp1025: return "cp1025";
	case koi8_u: return "koi8-u";
	case iso_8859_1: return "iso-8859-1";
	case iso_8859_2: return "iso-8859-2";
	case iso_8859_3: return "iso-8859-3";
	case iso_8859_4: return "iso-8859-4";
	case iso_8859_5: return "iso-8859-5";
	case iso_8859_6: return "iso-8859-6";
	case iso_8859_7: return "iso-8859-7";
	case iso_8859_8: return "iso-8859-8";
	case iso_8859_9: return "iso-8859-9";
	case iso_8859_13: return "iso-8859-13";
	case iso_8859_15: return "iso-8859-15";
	case x_europa: return "x-Europa";
	case iso_8859_8_i: return "iso-8859-8-i";
	case iso_2022_jp_no_halfwidth: return "iso-2022-jp";
	case csiso2022jp: return "csISO2022JP";
	case iso_2022_jp_jis_x_0201_1989: return "iso-2022-jp";
	case iso_2022_kr: return "iso-2022-kr";
	case x_cp50227: return "x-cp50227";
	case iso_2022_traditional_chinese: return "ISO 2022 Traditional Chinese";
	case ebcdic_japanese_katakana_extended: return "EBCDIC Japanese(Katakana) Extended";
	case ebcdic_us_canada_and_japanese: return "EBCDIC US - Canada and Japanese";
	case ebcdic_korean_extended_and_korean: return "EBCDIC Korean Extended and Korean";
	case ebcdic_simplified_chinese_extendedand_simplified_chinese:
		return "EBCDIC Simplified Chinese Extended and Simplified Chinese";
	case ebcdic_simplified_chinese: return "EBCDIC Simplified Chinese";
	case ebcdic_us_canada_and_traditional_chinese:
		return "EBCDIC US - Canada and Traditional Chinese";
	case ebcdic_japanese_latin_extendedand_japanese:
		return "EBCDIC Japanese(Latin) Extended and Japanese";
	case euc_jp: return "euc-jp";
	case euc_cn: return "EUC-CN";
	case euc_kr: return "euc-kr";
	case euc_traditional_chinese: return "EUC Traditional Chinese";
	case hz_gb_2312: return "hz-gb-2312";
	case gb18030: return "GB18030";
	case x_iscii_de: return "x-iscii-de";
	case x_iscii_be: return "x-iscii-be";
	case x_iscii_ta: return "x-iscii-ta";
	case x_iscii_te: return "x-iscii-te";
	case x_iscii_as: return "x-iscii-as";
	case x_iscii_or : return "x-iscii-or";
	case x_iscii_ka: return "x-iscii-ka";
	case x_iscii_ma: return "x-iscii-ma";
	case x_iscii_gu: return "x-iscii-gu";
	case x_iscii_pa: return "x-iscii-pa";
	case utf_7: return "utf-7";
	case utf_8: return "utf-8";
	default: return {};
	}
}

} //namespace

std::optional<std::string_view> get_cpid_name(code_page cpid) noexcept
{
	auto name = get_cpid_name_impl(cpid);
	return name.empty() ? std::optional<std::string_view>{} : name;
}

} //namespace pe_bliss::resources
