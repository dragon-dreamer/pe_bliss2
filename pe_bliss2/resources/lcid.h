#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

namespace pe_bliss::resources
{

// Language code ID
using lcid_type = std::uint32_t;

enum class lcid_release_key
{
	release_a, //Windows NT 3.51
	release_b, //Windows NT Server 4.0
	release_c, //Windows 2000
	release_d, //Windows XP and Windows Server 2003
	//Windows XP ELK v1 for Windows XP SP2, Windows Server 2003,
	//Windows Vista, and Windows Server 2008
	release_e1,
	//Windows XP ELK v2 for Windows XP SP2, Windows Server 2003,
	//Windows Vista, and Windows Server 2008
	release_e2,
	release_v, //Windows Server 2008 and Windows Vista
	release_7, //Windows 7 and Windows Server 2008 R2
	release_8, //Windows 8 and Windows Server 2012
	release_8_1, //Windows 8.1 and Windows Server 2012 R2
	release_10, //Windows 10 and Windows Server 2016
	release_10_1, //Windows 10 v1607 and Windows Server 2016
	release_10_2, //Windows 10 v1703
	release_10_3, //Windows 10 v1709
	release_10_4, //Windows 10 v1903
	release_10_5, //Windows 10 v2004
};

enum class lcid_language
{
	afrikaans,
	albanian,
	alsatian,
	amharic,
	arabic,
	armenian,
	assamese,
	azerbaijani_cyrillic,
	azerbaijani_latin,
	bangla,
	bashkir,
	basque,
	belarusian,
	bosnian_cyrillic,
	bosnian_latin,
	breton,
	bulgarian,
	burmese,
	catalan,
	central_atlas_tamazight_arabic,
	central_kurdish,
	cherokee,
	chinese_simplified,
	chinese_traditional,
	corsican,
	croatian,
	croatian_latin,
	czech,
	danish,
	dari,
	divehi,
	dutch,
	dzongkha,
	english,
	estonian,
	faroese,
	filipino,
	finnish,
	french,
	frisian,
	fulah_latin,
	fulah,
	galician,
	georgian,
	german,
	greek,
	greenlandic,
	guarani,
	gujarati,
	hausa_latin,
	hawaiian,
	hebrew,
	hindi,
	hungarian,
	icelandic,
	igbo,
	indonesian,
	inuktitut_latin,
	inuktitut_syllabics,
	irish,
	italian,
	japanese,
	kannada,
	kanuri_latin,
	kashmiri,
	kashmiri_devanagari,
	kazakh,
	khmer,
	k_iche,
	kinyarwanda,
	kiswahili,
	konkani,
	korean,
	kyrgyz,
	lao,
	latin,
	latvian,
	lithuanian,
	lower_sorbian,
	luxembourgish,
	macedonian,
	malay,
	malayalam,
	maltese,
	maori,
	mapudungun,
	marathi,
	mohawk,
	mongolian_cyrillic,
	mongolian_traditional_mongolian,
	nepali,
	norwegian_bokmal,
	norwegian_nynorsk,
	occitan,
	odia,
	oromo,
	pashto,
	persian,
	polish,
	portuguese,
	pseudo_language,
	punjabi,
	quechua,
	romanian,
	romansh,
	russian,
	sakha,
	sami_inari,
	sami_lule,
	sami_northern,
	sami_skolt,
	sami_southern,
	sanskrit,
	scottish_gaelic,
	serbian_cyrillic,
	serbian_latin,
	sesotho_sa_leboa,
	setswana,
	sindhi,
	sinhala,
	slovak,
	slovenian,
	somali,
	sotho,
	spanish,
	swedish,
	syriac,
	tajik_cyrillic,
	tamazight_latin,
	tamil,
	tatar,
	telugu,
	thai,
	tibetan,
	tigrinya,
	tsonga,
	turkish,
	turkmen,
	ukrainian,
	upper_sorbian,
	urdu,
	uyghur,
	uzbek_cyrillic,
	uzbek_latin,
	valencian,
	venda,
	vietnamese,
	welsh,
	wolof,
	xhosa,
	yi,
	yiddish,
	yoruba,
	zulu,
	neutral,
	process_default,
	system_default
};

[[nodiscard]]
std::optional<std::string_view> lcid_language_to_string(lcid_language lang) noexcept;

struct lcid_info
{
	lcid_type lcid;
	lcid_language language;
	std::string_view location;
	std::string_view language_tag;
	lcid_release_key release;
};

[[nodiscard]]
std::optional<lcid_info> get_lcid_info(lcid_type lcid) noexcept;

} //namespace pe_bliss::resources
