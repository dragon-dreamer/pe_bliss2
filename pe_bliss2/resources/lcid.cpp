#include "pe_bliss2/resources/lcid.h"

#include <array>
#include <string_view>
#include <unordered_map>

namespace pe_bliss::resources
{

std::optional<std::string_view> lcid_language_to_string(lcid_language lang) noexcept
{
	using namespace std::string_view_literals;
	static constexpr std::array language_names{
		"Afrikaans"sv,
		"Albanian"sv,
		"Alsatian"sv,
		"Amharic"sv,
		"Arabic"sv,
		"Armenian"sv,
		"Assamese"sv,
		"Azerbaijani (Cyrillic)"sv,
		"Azerbaijani (Latin)"sv,
		"Bangla"sv,
		"Bashkir"sv,
		"Basque"sv,
		"Belarusian"sv,
		"Bosnian (Cyrillic)"sv,
		"Bosnian (Latin)"sv,
		"Breton"sv,
		"Bulgarian"sv,
		"Burmese"sv,
		"Catalan"sv,
		"Central Atlas Tamazight (Arabic)"sv,
		"Central Kurdish"sv,
		"Cherokee"sv,
		"Chinese (Simplified)"sv,
		"Chinese (Traditional)"sv,
		"Corsican"sv,
		"Croatian"sv,
		"Croatian (Latin)"sv,
		"Czech"sv,
		"Danish"sv,
		"Dari"sv,
		"Divehi"sv,
		"Dutch"sv,
		"Dzongkha"sv,
		"English"sv,
		"Estonian"sv,
		"Faroese"sv,
		"Filipino"sv,
		"Finnish"sv,
		"French"sv,
		"Frisian"sv,
		"Fulah (Latin)"sv,
		"Fulah"sv,
		"Galician"sv,
		"Georgian"sv,
		"German"sv,
		"Greek"sv,
		"Greenlandic"sv,
		"Guarani"sv,
		"Gujarati"sv,
		"Hausa (Latin)"sv,
		"Hawaiian"sv,
		"Hebrew"sv,
		"Hindi"sv,
		"Hungarian"sv,
		"Icelandic"sv,
		"Igbo"sv,
		"Indonesian"sv,
		"Inuktitut (Latin)"sv,
		"Inuktitut (Syllabics)"sv,
		"Irish"sv,
		"Italian"sv,
		"Japanese"sv,
		"Kannada"sv,
		"Kanuri (Latin)"sv,
		"Kashmiri"sv,
		"Kashmiri (Devanagari)"sv,
		"Kazakh"sv,
		"Khmer"sv,
		"K'iche"sv,
		"Kinyarwanda"sv,
		"Kiswahili"sv,
		"Konkani"sv,
		"Korean"sv,
		"Kyrgyz"sv,
		"Lao"sv,
		"Latin"sv,
		"Latvian"sv,
		"Lithuanian"sv,
		"Lower Sorbian"sv,
		"Luxembourgish"sv,
		"Macedonian"sv,
		"Malay"sv,
		"Malayalam"sv,
		"Maltese"sv,
		"Maori"sv,
		"Mapudungun"sv,
		"Marathi"sv,
		"Mohawk"sv,
		"Mongolian (Cyrillic)"sv,
		"Mongolian (Traditional Mongolian)"sv,
		"Nepali"sv,
		"Norwegian (Bokmal)"sv,
		"Norwegian (Nynorsk)"sv,
		"Occitan"sv,
		"Odia"sv,
		"Oromo"sv,
		"Pashto"sv,
		"Persian"sv,
		"Polish"sv,
		"Portuguese"sv,
		"Pseudo Language"sv,
		"Punjabi"sv,
		"Quechua"sv,
		"Romanian"sv,
		"Romansh"sv,
		"Russian"sv,
		"Sakha"sv,
		"Sami (Inari)"sv,
		"Sami (Lule)"sv,
		"Sami (Northern)"sv,
		"Sami (Skolt)"sv,
		"Sami (Southern)"sv,
		"Sanskrit"sv,
		"Scottish Gaelic"sv,
		"Serbian (Cyrillic)"sv,
		"Serbian (Latin)"sv,
		"Sesotho sa Leboa"sv,
		"Setswana"sv,
		"Sindhi"sv,
		"Sinhala"sv,
		"Slovak"sv,
		"Slovenian"sv,
		"Somali"sv,
		"Sotho"sv,
		"Spanish"sv,
		"Swedish"sv,
		"Syriac"sv,
		"Tajik (Cyrillic)"sv,
		"Tamazight (Latin)"sv,
		"Tamil"sv,
		"Tatar"sv,
		"Telugu"sv,
		"Thai"sv,
		"Tibetan"sv,
		"Tigrinya"sv,
		"Tsonga"sv,
		"Turkish"sv,
		"Turkmen"sv,
		"Ukrainian"sv,
		"Upper Sorbian"sv,
		"Urdu"sv,
		"Uyghur"sv,
		"Uzbek (Cyrillic)"sv,
		"Uzbek (Latin)"sv,
		"Valencian"sv,
		"Venda"sv,
		"Vietnamese"sv,
		"Welsh"sv,
		"Wolof"sv,
		"Xhosa"sv,
		"Yi"sv,
		"Yiddish"sv,
		"Yoruba"sv,
		"Zulu"sv,
		"Neutral"sv,
		"Process Default"sv,
		"System Default"sv
	};

	const auto index = static_cast<std::size_t>(lang);
	if (index >= language_names.size())
		return {};

	return language_names[index];
}

namespace
{
// See https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-lcid/
lcid_info get_lcid_info_impl(lcid_type lcid) noexcept
{
	switch (lcid)
	{
	case 0x0036u: return { .lcid = lcid, .language = lcid_language::afrikaans, .language_tag = "af", .release = lcid_release_key::release_7 };
	case 0x0436u: return { .lcid = lcid, .language = lcid_language::afrikaans, .location = "South Africa", .language_tag = "af-ZA", .release = lcid_release_key::release_b };
	case 0x001Cu: return { .lcid = lcid, .language = lcid_language::albanian, .language_tag = "sq", .release = lcid_release_key::release_7 };
	case 0x041Cu: return { .lcid = lcid, .language = lcid_language::albanian, .location = "Albania", .language_tag = "sq-AL", .release = lcid_release_key::release_b };
	case 0x0084u: return { .lcid = lcid, .language = lcid_language::alsatian, .language_tag = "gsw", .release = lcid_release_key::release_7 };
	case 0x0484u: return { .lcid = lcid, .language = lcid_language::alsatian, .location = "France", .language_tag = "gsw-FR", .release = lcid_release_key::release_v };
	case 0x005Eu: return { .lcid = lcid, .language = lcid_language::amharic, .language_tag = "am", .release = lcid_release_key::release_7 };
	case 0x045Eu: return { .lcid = lcid, .language = lcid_language::amharic, .location = "Ethiopia", .language_tag = "am-ET", .release = lcid_release_key::release_v };
	case 0x0001u: return { .lcid = lcid, .language = lcid_language::arabic, .language_tag = "ar", .release = lcid_release_key::release_7 };
	case 0x1401u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Algeria", .language_tag = "ar-DZ", .release = lcid_release_key::release_b };
	case 0x3C01u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Bahrain", .language_tag = "ar-BH", .release = lcid_release_key::release_b };
	case 0x0c01u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Egypt", .language_tag = "ar-EG", .release = lcid_release_key::release_b };
	case 0x0801u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Iraq", .language_tag = "ar-IQ", .release = lcid_release_key::release_b };
	case 0x2C01u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Jordan", .language_tag = "ar-JO", .release = lcid_release_key::release_b };
	case 0x3401u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Kuwait", .language_tag = "ar-KW", .release = lcid_release_key::release_b };
	case 0x3001u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Lebanon", .language_tag = "ar-LB", .release = lcid_release_key::release_b };
	case 0x1001u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Libya", .language_tag = "ar-LY", .release = lcid_release_key::release_b };
	case 0x1801u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Morocco", .language_tag = "ar-MA", .release = lcid_release_key::release_b };
	case 0x2001u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Oman", .language_tag = "ar-OM", .release = lcid_release_key::release_b };
	case 0x4001u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Qatar", .language_tag = "ar-QA", .release = lcid_release_key::release_b };
	case 0x0401u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Saudi Arabia", .language_tag = "ar-SA", .release = lcid_release_key::release_b };
	case 0x2801u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Syria", .language_tag = "ar-SY", .release = lcid_release_key::release_b };
	case 0x1C01u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Tunisia", .language_tag = "ar-TN", .release = lcid_release_key::release_b };
	case 0x3801u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "U.A.E.", .language_tag = "ar-AE", .release = lcid_release_key::release_b };
	case 0x2401u: return { .lcid = lcid, .language = lcid_language::arabic, .location = "Yemen", .language_tag = "ar-YE", .release = lcid_release_key::release_b };
	case 0x002Bu: return { .lcid = lcid, .language = lcid_language::armenian, .language_tag = "hy", .release = lcid_release_key::release_7 };
	case 0x042Bu: return { .lcid = lcid, .language = lcid_language::armenian, .location = "Armenia", .language_tag = "hy-AM", .release = lcid_release_key::release_c };
	case 0x004Du: return { .lcid = lcid, .language = lcid_language::assamese, .language_tag = "as", .release = lcid_release_key::release_7 };
	case 0x044Du: return { .lcid = lcid, .language = lcid_language::assamese, .location = "India", .language_tag = "as-IN", .release = lcid_release_key::release_v };
	case 0x742Cu: return { .lcid = lcid, .language = lcid_language::azerbaijani_cyrillic, .language_tag = "az-Cyrl", .release = lcid_release_key::release_7 };
	case 0x082Cu: return { .lcid = lcid, .language = lcid_language::azerbaijani_cyrillic, .location = "Azerbaijan", .language_tag = "az-Cyrl-AZ", .release = lcid_release_key::release_c };
	case 0x002Cu: return { .lcid = lcid, .language = lcid_language::azerbaijani_latin, .language_tag = "az", .release = lcid_release_key::release_7 };
	case 0x782Cu: return { .lcid = lcid, .language = lcid_language::azerbaijani_latin, .language_tag = "az-Latn", .release = lcid_release_key::release_7 };
	case 0x042Cu: return { .lcid = lcid, .language = lcid_language::azerbaijani_latin, .location = "Azerbaijan", .language_tag = "az-Latn-AZ", .release = lcid_release_key::release_c };
	case 0x0045u: return { .lcid = lcid, .language = lcid_language::bangla, .language_tag = "bn", .release = lcid_release_key::release_7 };
	case 0x0845u: return { .lcid = lcid, .language = lcid_language::bangla, .location = "Bangladesh", .language_tag = "bn-BD", .release = lcid_release_key::release_v };
	case 0x0445u: return { .lcid = lcid, .language = lcid_language::bangla, .location = "India", .language_tag = "bn-IN", .release = lcid_release_key::release_e1 };
	case 0x006Du: return { .lcid = lcid, .language = lcid_language::bashkir, .language_tag = "ba", .release = lcid_release_key::release_7 };
	case 0x046Du: return { .lcid = lcid, .language = lcid_language::bashkir, .location = "Russia", .language_tag = "ba-RU", .release = lcid_release_key::release_v };
	case 0x002Du: return { .lcid = lcid, .language = lcid_language::basque, .language_tag = "eu", .release = lcid_release_key::release_7 };
	case 0x042Du: return { .lcid = lcid, .language = lcid_language::basque, .location = "Spain", .language_tag = "eu-ES", .release = lcid_release_key::release_b };
	case 0x0023u: return { .lcid = lcid, .language = lcid_language::belarusian, .language_tag = "be", .release = lcid_release_key::release_7 };
	case 0x0423u: return { .lcid = lcid, .language = lcid_language::belarusian, .location = "Belarus", .language_tag = "be-BY", .release = lcid_release_key::release_b };
	case 0x641Au: return { .lcid = lcid, .language = lcid_language::bosnian_cyrillic, .language_tag = "bs-Cyrl", .release = lcid_release_key::release_7 };
	case 0x201Au: return { .lcid = lcid, .language = lcid_language::bosnian_cyrillic, .location = "Bosnia and Herzegovina", .language_tag = "bs-Cyrl-BA", .release = lcid_release_key::release_e1 };
	case 0x681Au: return { .lcid = lcid, .language = lcid_language::bosnian_latin, .language_tag = "bs-Latn", .release = lcid_release_key::release_7 };
	case 0x781Au: return { .lcid = lcid, .language = lcid_language::bosnian_latin, .language_tag = "bs", .release = lcid_release_key::release_7 };
	case 0x141Au: return { .lcid = lcid, .language = lcid_language::bosnian_latin, .location = "Bosnia and Herzegovina", .language_tag = "bs-Latn-BA", .release = lcid_release_key::release_e1 };
	case 0x007Eu: return { .lcid = lcid, .language = lcid_language::breton, .language_tag = "br", .release = lcid_release_key::release_7 };
	case 0x047Eu: return { .lcid = lcid, .language = lcid_language::breton, .location = "France", .language_tag = "br-FR", .release = lcid_release_key::release_v };
	case 0x0002u: return { .lcid = lcid, .language = lcid_language::bulgarian, .language_tag = "bg", .release = lcid_release_key::release_7 };
	case 0x0402u: return { .lcid = lcid, .language = lcid_language::bulgarian, .location = "Bulgaria", .language_tag = "bg-BG", .release = lcid_release_key::release_b };
	case 0x0055u: return { .lcid = lcid, .language = lcid_language::burmese, .language_tag = "my", .release = lcid_release_key::release_8_1 };
	case 0x0455u: return { .lcid = lcid, .language = lcid_language::burmese, .location = "Myanmar", .language_tag = "my-MM", .release = lcid_release_key::release_8_1 };
	case 0x0003u: return { .lcid = lcid, .language = lcid_language::catalan, .language_tag = "ca", .release = lcid_release_key::release_7 };
	case 0x0403u: return { .lcid = lcid, .language = lcid_language::catalan, .location = "Spain", .language_tag = "ca-ES", .release = lcid_release_key::release_b };
	case 0x045Fu: return { .lcid = lcid, .language = lcid_language::central_atlas_tamazight_arabic, .location = "Morocco", .language_tag = "tzm-ArabMA", .release = lcid_release_key::release_10 };
	case 0x0092u: return { .lcid = lcid, .language = lcid_language::central_kurdish, .language_tag = "ku", .release = lcid_release_key::release_8 };
	case 0x7c92u: return { .lcid = lcid, .language = lcid_language::central_kurdish, .language_tag = "ku-Arab", .release = lcid_release_key::release_8 };
	case 0x0492u: return { .lcid = lcid, .language = lcid_language::central_kurdish, .location = "Iraq", .language_tag = "ku-Arab-IQ", .release = lcid_release_key::release_8 };
	case 0x005Cu: return { .lcid = lcid, .language = lcid_language::cherokee, .language_tag = "chr", .release = lcid_release_key::release_8 };
	case 0x7c5Cu: return { .lcid = lcid, .language = lcid_language::cherokee, .language_tag = "chr-Cher", .release = lcid_release_key::release_8 };
	case 0x045Cu: return { .lcid = lcid, .language = lcid_language::cherokee, .location = "United States", .language_tag = "chr-Cher-US", .release = lcid_release_key::release_8 };
	case 0x0004u: return { .lcid = lcid, .language = lcid_language::chinese_simplified, .language_tag = "zh-Hans", .release = lcid_release_key::release_a };
	case 0x7804u: return { .lcid = lcid, .language = lcid_language::chinese_simplified, .language_tag = "zh", .release = lcid_release_key::release_7 };
	case 0x0804u: return { .lcid = lcid, .language = lcid_language::chinese_simplified, .location = "People's Republic of China", .language_tag = "zh-CN", .release = lcid_release_key::release_a };
	case 0x1004u: return { .lcid = lcid, .language = lcid_language::chinese_simplified, .location = "Singapore", .language_tag = "zh-SG", .release = lcid_release_key::release_a };
	case 0x7C04u: return { .lcid = lcid, .language = lcid_language::chinese_traditional, .language_tag = "zh-Hant", .release = lcid_release_key::release_a };
	case 0x0C04u: return { .lcid = lcid, .language = lcid_language::chinese_traditional, .location = "Hong Kong S.A.R.", .language_tag = "zh-HK", .release = lcid_release_key::release_a };
	case 0x1404u: return { .lcid = lcid, .language = lcid_language::chinese_traditional, .location = "Macao S.A.R.", .language_tag = "zh-MO", .release = lcid_release_key::release_d };
	case 0x0404u: return { .lcid = lcid, .language = lcid_language::chinese_traditional, .location = "Taiwan", .language_tag = "zh-TW", .release = lcid_release_key::release_a };
	case 0x0083u: return { .lcid = lcid, .language = lcid_language::corsican, .language_tag = "co", .release = lcid_release_key::release_7 };
	case 0x0483u: return { .lcid = lcid, .language = lcid_language::corsican, .location = "France", .language_tag = "co-FR", .release = lcid_release_key::release_v };
	case 0x001Au: return { .lcid = lcid, .language = lcid_language::croatian, .language_tag = "hr", .release = lcid_release_key::release_7 };
	case 0x041Au: return { .lcid = lcid, .language = lcid_language::croatian, .location = "Croatia", .language_tag = "hr-HR", .release = lcid_release_key::release_a };
	case 0x101Au: return { .lcid = lcid, .language = lcid_language::croatian_latin, .location = "Bosnia and Herzegovina", .language_tag = "hr-BA", .release = lcid_release_key::release_e1 };
	case 0x0005u: return { .lcid = lcid, .language = lcid_language::czech, .language_tag = "cs", .release = lcid_release_key::release_7 };
	case 0x0405u: return { .lcid = lcid, .language = lcid_language::czech, .location = "Czech Republic", .language_tag = "cs-CZ", .release = lcid_release_key::release_a };
	case 0x0006u: return { .lcid = lcid, .language = lcid_language::danish, .language_tag = "da", .release = lcid_release_key::release_7 };
	case 0x0406u: return { .lcid = lcid, .language = lcid_language::danish, .location = "Denmark", .language_tag = "da-DK", .release = lcid_release_key::release_a };
	case 0x008Cu: return { .lcid = lcid, .language = lcid_language::dari, .language_tag = "prs", .release = lcid_release_key::release_7 };
	case 0x048Cu: return { .lcid = lcid, .language = lcid_language::dari, .location = "Afghanistan", .language_tag = "prs-AF", .release = lcid_release_key::release_v };
	case 0x0065u: return { .lcid = lcid, .language = lcid_language::divehi, .language_tag = "dv", .release = lcid_release_key::release_7 };
	case 0x0465u: return { .lcid = lcid, .language = lcid_language::divehi, .location = "Maldives", .language_tag = "dv-MV", .release = lcid_release_key::release_d };
	case 0x0013u: return { .lcid = lcid, .language = lcid_language::dutch, .language_tag = "nl", .release = lcid_release_key::release_7 };
	case 0x0813u: return { .lcid = lcid, .language = lcid_language::dutch, .location = "Belgium", .language_tag = "nl-BE", .release = lcid_release_key::release_a };
	case 0x0413u: return { .lcid = lcid, .language = lcid_language::dutch, .location = "Netherlands", .language_tag = "nl-NL", .release = lcid_release_key::release_a };
	case 0x0C51u: return { .lcid = lcid, .language = lcid_language::dzongkha, .location = "Bhutan", .language_tag = "dz-BT", .release = lcid_release_key::release_10 };
	case 0x0009u: return { .lcid = lcid, .language = lcid_language::english, .language_tag = "en", .release = lcid_release_key::release_7 };
	case 0x0C09u: return { .lcid = lcid, .language = lcid_language::english, .location = "Australia", .language_tag = "en-AU", .release = lcid_release_key::release_a };
	case 0x2809u: return { .lcid = lcid, .language = lcid_language::english, .location = "Belize", .language_tag = "en-BZ", .release = lcid_release_key::release_b };
	case 0x1009u: return { .lcid = lcid, .language = lcid_language::english, .location = "Canada", .language_tag = "en-CA", .release = lcid_release_key::release_a };
	case 0x2409u: return { .lcid = lcid, .language = lcid_language::english, .location = "Caribbean", .language_tag = "en-029", .release = lcid_release_key::release_b };
	case 0x3C09u: return { .lcid = lcid, .language = lcid_language::english, .location = "Hong Kong", .language_tag = "en-HK", .release = lcid_release_key::release_8_1 };
	case 0x4009u: return { .lcid = lcid, .language = lcid_language::english, .location = "India", .language_tag = "en-IN", .release = lcid_release_key::release_v };
	case 0x1809u: return { .lcid = lcid, .language = lcid_language::english, .location = "Ireland", .language_tag = "en-IE", .release = lcid_release_key::release_a };
	case 0x2009u: return { .lcid = lcid, .language = lcid_language::english, .location = "Jamaica", .language_tag = "en-JM", .release = lcid_release_key::release_b };
	case 0x4409u: return { .lcid = lcid, .language = lcid_language::english, .location = "Malaysia", .language_tag = "en-MY", .release = lcid_release_key::release_v };
	case 0x1409u: return { .lcid = lcid, .language = lcid_language::english, .location = "New Zealand", .language_tag = "en-NZ", .release = lcid_release_key::release_a };
	case 0x3409u: return { .lcid = lcid, .language = lcid_language::english, .location = "Republic of the Philippines", .language_tag = "en-PH", .release = lcid_release_key::release_c };
	case 0x4809u: return { .lcid = lcid, .language = lcid_language::english, .location = "Singapore", .language_tag = "en-SG", .release = lcid_release_key::release_v };
	case 0x1C09u: return { .lcid = lcid, .language = lcid_language::english, .location = "South Africa", .language_tag = "en-ZA", .release = lcid_release_key::release_b };
	case 0x2c09u: return { .lcid = lcid, .language = lcid_language::english, .location = "Trinidad and Tobago", .language_tag = "en-TT", .release = lcid_release_key::release_b };
	case 0x4C09u: return { .lcid = lcid, .language = lcid_language::english, .location = "United Arab Emirates", .language_tag = "en-AE", .release = lcid_release_key::release_10_5 };
	case 0x0809u: return { .lcid = lcid, .language = lcid_language::english, .location = "United Kingdom", .language_tag = "en-GB", .release = lcid_release_key::release_a };
	case 0x0409u: return { .lcid = lcid, .language = lcid_language::english, .location = "United States", .language_tag = "en-US", .release = lcid_release_key::release_a };
	case 0x3009u: return { .lcid = lcid, .language = lcid_language::english, .location = "Zimbabwe", .language_tag = "en-ZW", .release = lcid_release_key::release_c };
	case 0x0025u: return { .lcid = lcid, .language = lcid_language::estonian, .language_tag = "et", .release = lcid_release_key::release_7 };
	case 0x0425u: return { .lcid = lcid, .language = lcid_language::estonian, .location = "Estonia", .language_tag = "et-EE", .release = lcid_release_key::release_b };
	case 0x0038u: return { .lcid = lcid, .language = lcid_language::faroese, .language_tag = "fo", .release = lcid_release_key::release_7 };
	case 0x0438u: return { .lcid = lcid, .language = lcid_language::faroese, .location = "Faroe Islands", .language_tag = "fo-FO", .release = lcid_release_key::release_b };
	case 0x0064u: return { .lcid = lcid, .language = lcid_language::filipino, .language_tag = "fil", .release = lcid_release_key::release_7 };
	case 0x0464u: return { .lcid = lcid, .language = lcid_language::filipino, .location = "Philippines", .language_tag = "fil-PH", .release = lcid_release_key::release_e2 };
	case 0x000Bu: return { .lcid = lcid, .language = lcid_language::finnish, .language_tag = "fi", .release = lcid_release_key::release_7 };
	case 0x040Bu: return { .lcid = lcid, .language = lcid_language::finnish, .location = "Finland", .language_tag = "fi-FI", .release = lcid_release_key::release_a };
	case 0x000Cu: return { .lcid = lcid, .language = lcid_language::french, .language_tag = "fr", .release = lcid_release_key::release_7 };
	case 0x080Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Belgium", .language_tag = "fr-BE", .release = lcid_release_key::release_a };
	case 0x2c0Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Cameroon", .language_tag = "fr-CM", .release = lcid_release_key::release_8_1 };
	case 0x0c0Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Canada", .language_tag = "fr-CA", .release = lcid_release_key::release_a };
	case 0x1C0Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Caribbean", .language_tag = "fr-029", .release = lcid_release_key::release_10 };
	case 0x240Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Congo, DRC", .language_tag = "fr-CD", .release = lcid_release_key::release_8_1 };
	case 0x300Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Cote d'Ivoire", .language_tag = "fr-CI", .release = lcid_release_key::release_8_1 };
	case 0x040Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "France", .language_tag = "fr-FR", .release = lcid_release_key::release_a };
	case 0x3c0Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Haiti", .language_tag = "fr-HT", .release = lcid_release_key::release_8_1 };
	case 0x140Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Luxembourg", .language_tag = "fr-LU", .release = lcid_release_key::release_a };
	case 0x340Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Mali", .language_tag = "fr-ML", .release = lcid_release_key::release_8_1 };
	case 0x380Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Morocco", .language_tag = "fr-MA", .release = lcid_release_key::release_8_1 };
	case 0x180Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Principality of Monaco", .language_tag = "fr-MC", .release = lcid_release_key::release_a };
	case 0x200Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Reunion", .language_tag = "fr-RE", .release = lcid_release_key::release_8_1 };
	case 0x280Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Senegal", .language_tag = "fr-SN", .release = lcid_release_key::release_8_1 };
	case 0x100Cu: return { .lcid = lcid, .language = lcid_language::french, .location = "Switzerland", .language_tag = "fr-CH", .release = lcid_release_key::release_a };
	case 0x0062u: return { .lcid = lcid, .language = lcid_language::frisian, .language_tag = "fy", .release = lcid_release_key::release_7 };
	case 0x0462u: return { .lcid = lcid, .language = lcid_language::frisian, .location = "Netherlands", .language_tag = "fy-NL", .release = lcid_release_key::release_e2 };
	case 0x0067u: return { .lcid = lcid, .language = lcid_language::fulah, .language_tag = "ff", .release = lcid_release_key::release_8 };
	case 0x7C67u: return { .lcid = lcid, .language = lcid_language::fulah_latin, .language_tag = "ff-Latn", .release = lcid_release_key::release_8 };
	case 0x0467u: return { .lcid = lcid, .language = lcid_language::fulah, .location = "Nigeria", .language_tag = "ff-NG", .release = lcid_release_key::release_10 };
	case 0x0867u: return { .lcid = lcid, .language = lcid_language::fulah, .location = "Senegal", .language_tag = "ff-Latn-SN", .release = lcid_release_key::release_8 };
	case 0x0056u: return { .lcid = lcid, .language = lcid_language::galician, .language_tag = "gl", .release = lcid_release_key::release_7 };
	case 0x0456u: return { .lcid = lcid, .language = lcid_language::galician, .location = "Spain", .language_tag = "gl-ES", .release = lcid_release_key::release_d };
	case 0x0037u: return { .lcid = lcid, .language = lcid_language::georgian, .language_tag = "ka", .release = lcid_release_key::release_7 };
	case 0x0437u: return { .lcid = lcid, .language = lcid_language::georgian, .location = "Georgia", .language_tag = "ka-GE", .release = lcid_release_key::release_c };
	case 0x0007u: return { .lcid = lcid, .language = lcid_language::german, .language_tag = "de", .release = lcid_release_key::release_7 };
	case 0x0C07u: return { .lcid = lcid, .language = lcid_language::german, .location = "Austria", .language_tag = "de-AT", .release = lcid_release_key::release_a };
	case 0x0407u: return { .lcid = lcid, .language = lcid_language::german, .location = "Germany", .language_tag = "de-DE", .release = lcid_release_key::release_a };
	case 0x1407u: return { .lcid = lcid, .language = lcid_language::german, .location = "Liechtenstein", .language_tag = "de-LI", .release = lcid_release_key::release_b };
	case 0x1007u: return { .lcid = lcid, .language = lcid_language::german, .location = "Luxembourg", .language_tag = "de-LU", .release = lcid_release_key::release_b };
	case 0x0807u: return { .lcid = lcid, .language = lcid_language::german, .location = "Switzerland", .language_tag = "de-CH", .release = lcid_release_key::release_a };
	case 0x0008u: return { .lcid = lcid, .language = lcid_language::greek, .language_tag = "el", .release = lcid_release_key::release_7 };
	case 0x0408u: return { .lcid = lcid, .language = lcid_language::greek, .location = "Greece", .language_tag = "el-GR", .release = lcid_release_key::release_a };
	case 0x006Fu: return { .lcid = lcid, .language = lcid_language::greenlandic, .language_tag = "kl", .release = lcid_release_key::release_7 };
	case 0x046Fu: return { .lcid = lcid, .language = lcid_language::greenlandic, .location = "Greenland", .language_tag = "kl-GL", .release = lcid_release_key::release_v };
	case 0x0074u: return { .lcid = lcid, .language = lcid_language::guarani, .language_tag = "gn", .release = lcid_release_key::release_8_1 };
	case 0x0474u: return { .lcid = lcid, .language = lcid_language::guarani, .location = "Paraguay", .language_tag = "gn-PY", .release = lcid_release_key::release_8_1 };
	case 0x0047u: return { .lcid = lcid, .language = lcid_language::gujarati, .language_tag = "gu", .release = lcid_release_key::release_7 };
	case 0x0447u: return { .lcid = lcid, .language = lcid_language::gujarati, .location = "India", .language_tag = "gu-IN", .release = lcid_release_key::release_d };
	case 0x0068u: return { .lcid = lcid, .language = lcid_language::hausa_latin, .language_tag = "ha", .release = lcid_release_key::release_7 };
	case 0x7C68u: return { .lcid = lcid, .language = lcid_language::hausa_latin, .language_tag = "ha-Latn", .release = lcid_release_key::release_7 };
	case 0x0468u: return { .lcid = lcid, .language = lcid_language::hausa_latin, .location = "Nigeria", .language_tag = "ha-Latn-NG", .release = lcid_release_key::release_v };
	case 0x0075u: return { .lcid = lcid, .language = lcid_language::hawaiian, .language_tag = "haw", .release = lcid_release_key::release_8 };
	case 0x0475u: return { .lcid = lcid, .language = lcid_language::hawaiian, .location = "United States", .language_tag = "haw-US", .release = lcid_release_key::release_8 };
	case 0x000Du: return { .lcid = lcid, .language = lcid_language::hebrew, .language_tag = "he", .release = lcid_release_key::release_7 };
	case 0x040Du: return { .lcid = lcid, .language = lcid_language::hebrew, .location = "Israel", .language_tag = "he-IL", .release = lcid_release_key::release_b };
	case 0x0039u: return { .lcid = lcid, .language = lcid_language::hindi, .language_tag = "hi", .release = lcid_release_key::release_7 };
	case 0x0439u: return { .lcid = lcid, .language = lcid_language::hindi, .location = "India", .language_tag = "hi-IN", .release = lcid_release_key::release_c };
	case 0x000Eu: return { .lcid = lcid, .language = lcid_language::hungarian, .language_tag = "hu", .release = lcid_release_key::release_7 };
	case 0x040Eu: return { .lcid = lcid, .language = lcid_language::hungarian, .location = "Hungary", .language_tag = "hu-HU", .release = lcid_release_key::release_a };
	case 0x000Fu: return { .lcid = lcid, .language = lcid_language::icelandic, .language_tag = "is", .release = lcid_release_key::release_7 };
	case 0x040Fu: return { .lcid = lcid, .language = lcid_language::icelandic, .location = "Iceland", .language_tag = "is-IS", .release = lcid_release_key::release_a };
	case 0x0070u: return { .lcid = lcid, .language = lcid_language::igbo, .language_tag = "ig", .release = lcid_release_key::release_7 };
	case 0x0470u: return { .lcid = lcid, .language = lcid_language::igbo, .location = "Nigeria", .language_tag = "ig-NG", .release = lcid_release_key::release_v };
	case 0x0021u: return { .lcid = lcid, .language = lcid_language::indonesian, .language_tag = "id", .release = lcid_release_key::release_7 };
	case 0x0421u: return { .lcid = lcid, .language = lcid_language::indonesian, .location = "Indonesia", .language_tag = "id-ID", .release = lcid_release_key::release_b };
	case 0x005Du: return { .lcid = lcid, .language = lcid_language::inuktitut_latin, .language_tag = "iu", .release = lcid_release_key::release_7 };
	case 0x7C5Du: return { .lcid = lcid, .language = lcid_language::inuktitut_latin, .language_tag = "iu-Latn", .release = lcid_release_key::release_7 };
	case 0x085Du: return { .lcid = lcid, .language = lcid_language::inuktitut_latin, .location = "Canada", .language_tag = "iu-Latn-CA", .release = lcid_release_key::release_e2 };
	case 0x785Du: return { .lcid = lcid, .language = lcid_language::inuktitut_syllabics, .language_tag = "iu-Cans", .release = lcid_release_key::release_7 };
	case 0x045du: return { .lcid = lcid, .language = lcid_language::inuktitut_syllabics, .location = "Canada", .language_tag = "iu-Cans-CA", .release = lcid_release_key::release_v };
	case 0x003Cu: return { .lcid = lcid, .language = lcid_language::irish, .language_tag = "ga", .release = lcid_release_key::release_7 };
	case 0x083Cu: return { .lcid = lcid, .language = lcid_language::irish, .location = "Ireland", .language_tag = "ga-IE", .release = lcid_release_key::release_e2 };
	case 0x0010u: return { .lcid = lcid, .language = lcid_language::italian, .language_tag = "it", .release = lcid_release_key::release_7 };
	case 0x0410u: return { .lcid = lcid, .language = lcid_language::italian, .location = "Italy", .language_tag = "it-IT", .release = lcid_release_key::release_a };
	case 0x0810u: return { .lcid = lcid, .language = lcid_language::italian, .location = "Switzerland", .language_tag = "it-CH", .release = lcid_release_key::release_a };
	case 0x0011u: return { .lcid = lcid, .language = lcid_language::japanese, .language_tag = "ja", .release = lcid_release_key::release_7 };
	case 0x0411u: return { .lcid = lcid, .language = lcid_language::japanese, .location = "Japan", .language_tag = "ja-JP", .release = lcid_release_key::release_a };
	case 0x004Bu: return { .lcid = lcid, .language = lcid_language::kannada, .language_tag = "kn", .release = lcid_release_key::release_7 };
	case 0x044Bu: return { .lcid = lcid, .language = lcid_language::kannada, .location = "India", .language_tag = "kn-IN", .release = lcid_release_key::release_d };
	case 0x0471u: return { .lcid = lcid, .language = lcid_language::kanuri_latin, .location = "Nigeria", .language_tag = "kr-Latn-NG", .release = lcid_release_key::release_10 };
	case 0x0060u: return { .lcid = lcid, .language = lcid_language::kashmiri, .language_tag = "ks", .release = lcid_release_key::release_10 };
	case 0x0460u: return { .lcid = lcid, .language = lcid_language::kashmiri, .location = "Perso-Arabic", .language_tag = "ks-Arab", .release = lcid_release_key::release_10 };
	case 0x0860u: return { .lcid = lcid, .language = lcid_language::kashmiri_devanagari, .location = "India", .language_tag = "ks-Deva-IN", .release = lcid_release_key::release_10 };
	case 0x003Fu: return { .lcid = lcid, .language = lcid_language::kazakh, .language_tag = "kk", .release = lcid_release_key::release_7 };
	case 0x043Fu: return { .lcid = lcid, .language = lcid_language::kazakh, .location = "Kazakhstan", .language_tag = "kk-KZ", .release = lcid_release_key::release_c };
	case 0x0053u: return { .lcid = lcid, .language = lcid_language::khmer, .language_tag = "km", .release = lcid_release_key::release_7 };
	case 0x0453u: return { .lcid = lcid, .language = lcid_language::khmer, .location = "Cambodia", .language_tag = "km-KH", .release = lcid_release_key::release_v };
	case 0x0086u: return { .lcid = lcid, .language = lcid_language::k_iche, .language_tag = "quc", .release = lcid_release_key::release_10 };
	case 0x0486u: return { .lcid = lcid, .language = lcid_language::k_iche, .location = "Guatemala", .language_tag = "quc-Latn-GT", .release = lcid_release_key::release_10 };
	case 0x0087u: return { .lcid = lcid, .language = lcid_language::kinyarwanda, .language_tag = "rw", .release = lcid_release_key::release_7 };
	case 0x0487u: return { .lcid = lcid, .language = lcid_language::kinyarwanda, .location = "Rwanda", .language_tag = "rw-RW", .release = lcid_release_key::release_v };
	case 0x0041u: return { .lcid = lcid, .language = lcid_language::kiswahili, .language_tag = "sw", .release = lcid_release_key::release_7 };
	case 0x0441u: return { .lcid = lcid, .language = lcid_language::kiswahili, .location = "Kenya", .language_tag = "sw-KE", .release = lcid_release_key::release_c };
	case 0x0057u: return { .lcid = lcid, .language = lcid_language::konkani, .language_tag = "kok", .release = lcid_release_key::release_7 };
	case 0x0457u: return { .lcid = lcid, .language = lcid_language::konkani, .location = "India", .language_tag = "kok-IN", .release = lcid_release_key::release_c };
	case 0x0012u: return { .lcid = lcid, .language = lcid_language::korean, .language_tag = "ko", .release = lcid_release_key::release_7 };
	case 0x0412u: return { .lcid = lcid, .language = lcid_language::korean, .location = "Korea", .language_tag = "ko-KR", .release = lcid_release_key::release_a };
	case 0x0040u: return { .lcid = lcid, .language = lcid_language::kyrgyz, .language_tag = "ky", .release = lcid_release_key::release_7 };
	case 0x0440u: return { .lcid = lcid, .language = lcid_language::kyrgyz, .location = "Kyrgyzstan", .language_tag = "ky-KG", .release = lcid_release_key::release_d };
	case 0x0054u: return { .lcid = lcid, .language = lcid_language::lao, .language_tag = "lo", .release = lcid_release_key::release_7 };
	case 0x0454u: return { .lcid = lcid, .language = lcid_language::lao, .location = "Lao P.D.R.", .language_tag = "lo-LA", .release = lcid_release_key::release_v };
	case 0x0476u: return { .lcid = lcid, .language = lcid_language::latin, .location = "Vatican City", .language_tag = "la-VA", .release = lcid_release_key::release_10_5 };
	case 0x0026u: return { .lcid = lcid, .language = lcid_language::latvian, .language_tag = "lv", .release = lcid_release_key::release_7 };
	case 0x0426u: return { .lcid = lcid, .language = lcid_language::latvian, .location = "Latvia", .language_tag = "lv-LV", .release = lcid_release_key::release_b };
	case 0x0027u: return { .lcid = lcid, .language = lcid_language::lithuanian, .language_tag = "lt", .release = lcid_release_key::release_7 };
	case 0x0427u: return { .lcid = lcid, .language = lcid_language::lithuanian, .location = "Lithuania", .language_tag = "lt-LT", .release = lcid_release_key::release_b };
	case 0x7C2Eu: return { .lcid = lcid, .language = lcid_language::lower_sorbian, .language_tag = "dsb", .release = lcid_release_key::release_7 };
	case 0x082Eu: return { .lcid = lcid, .language = lcid_language::lower_sorbian, .location = "Germany", .language_tag = "dsb-DE", .release = lcid_release_key::release_v };
	case 0x006Eu: return { .lcid = lcid, .language = lcid_language::luxembourgish, .language_tag = "lb", .release = lcid_release_key::release_7 };
	case 0x046Eu: return { .lcid = lcid, .language = lcid_language::luxembourgish, .location = "Luxembourg", .language_tag = "lb-LU", .release = lcid_release_key::release_e2 };
	case 0x002Fu: return { .lcid = lcid, .language = lcid_language::macedonian, .language_tag = "mk", .release = lcid_release_key::release_7 };
	case 0x042Fu: return { .lcid = lcid, .language = lcid_language::macedonian, .location = "North Macedonia ", .language_tag = "mk-MK", .release = lcid_release_key::release_c };
	case 0x003Eu: return { .lcid = lcid, .language = lcid_language::malay, .language_tag = "ms", .release = lcid_release_key::release_7 };
	case 0x083Eu: return { .lcid = lcid, .language = lcid_language::malay, .location = "Brunei Darussalam", .language_tag = "ms-BN", .release = lcid_release_key::release_c };
	case 0x043Eu: return { .lcid = lcid, .language = lcid_language::malay, .location = "Malaysia", .language_tag = "ms-MY", .release = lcid_release_key::release_c };
	case 0x004Cu: return { .lcid = lcid, .language = lcid_language::malayalam, .language_tag = "ml", .release = lcid_release_key::release_7 };
	case 0x044Cu: return { .lcid = lcid, .language = lcid_language::malayalam, .location = "India", .language_tag = "ml-IN", .release = lcid_release_key::release_e1 };
	case 0x003Au: return { .lcid = lcid, .language = lcid_language::maltese, .language_tag = "mt", .release = lcid_release_key::release_7 };
	case 0x043Au: return { .lcid = lcid, .language = lcid_language::maltese, .location = "Malta", .language_tag = "mt-MT", .release = lcid_release_key::release_e1 };
	case 0x0081u: return { .lcid = lcid, .language = lcid_language::maori, .language_tag = "mi", .release = lcid_release_key::release_7 };
	case 0x0481u: return { .lcid = lcid, .language = lcid_language::maori, .location = "New Zealand", .language_tag = "mi-NZ", .release = lcid_release_key::release_e1 };
	case 0x007Au: return { .lcid = lcid, .language = lcid_language::mapudungun, .language_tag = "arn", .release = lcid_release_key::release_7 };
	case 0x047Au: return { .lcid = lcid, .language = lcid_language::mapudungun, .location = "Chile", .language_tag = "arn-CL", .release = lcid_release_key::release_e2 };
	case 0x004Eu: return { .lcid = lcid, .language = lcid_language::marathi, .language_tag = "mr", .release = lcid_release_key::release_7 };
	case 0x044Eu: return { .lcid = lcid, .language = lcid_language::marathi, .location = "India", .language_tag = "mr-IN", .release = lcid_release_key::release_c };
	case 0x007Cu: return { .lcid = lcid, .language = lcid_language::mohawk, .language_tag = "moh", .release = lcid_release_key::release_7 };
	case 0x047Cu: return { .lcid = lcid, .language = lcid_language::mohawk, .location = "Canada", .language_tag = "moh-CA", .release = lcid_release_key::release_e2 };
	case 0x0050u: return { .lcid = lcid, .language = lcid_language::mongolian_cyrillic, .language_tag = "mn", .release = lcid_release_key::release_7 };
	case 0x7850u: return { .lcid = lcid, .language = lcid_language::mongolian_cyrillic, .language_tag = "mn-Cyrl", .release = lcid_release_key::release_7 };
	case 0x0450u: return { .lcid = lcid, .language = lcid_language::mongolian_cyrillic, .location = "Mongolia", .language_tag = "mn-MN", .release = lcid_release_key::release_d };
	case 0x7C50u: return { .lcid = lcid, .language = lcid_language::mongolian_traditional_mongolian, .language_tag = "mn-Mong", .release = lcid_release_key::release_7 };
	case 0x0850u: return { .lcid = lcid, .language = lcid_language::mongolian_traditional_mongolian, .location = "People's Republic of China", .language_tag = "mn-MongCN", .release = lcid_release_key::release_v };
	case 0x0C50u: return { .lcid = lcid, .language = lcid_language::mongolian_traditional_mongolian, .location = "Mongolia", .language_tag = "mn-MongMN", .release = lcid_release_key::release_7 };
	case 0x0061u: return { .lcid = lcid, .language = lcid_language::nepali, .language_tag = "ne", .release = lcid_release_key::release_7 };
	case 0x0861u: return { .lcid = lcid, .language = lcid_language::nepali, .location = "India", .language_tag = "ne-IN", .release = lcid_release_key::release_8_1 };
	case 0x0461u: return { .lcid = lcid, .language = lcid_language::nepali, .location = "Nepal", .language_tag = "ne-NP", .release = lcid_release_key::release_e2 };
	case 0x0014u: return { .lcid = lcid, .language = lcid_language::norwegian_bokmal, .language_tag = "no", .release = lcid_release_key::release_7 };
	case 0x7C14u: return { .lcid = lcid, .language = lcid_language::norwegian_bokmal, .language_tag = "nb", .release = lcid_release_key::release_7 };
	case 0x0414u: return { .lcid = lcid, .language = lcid_language::norwegian_bokmal, .location = "Norway", .language_tag = "nb-NO", .release = lcid_release_key::release_a };
	case 0x7814u: return { .lcid = lcid, .language = lcid_language::norwegian_nynorsk, .language_tag = "nn", .release = lcid_release_key::release_7 };
	case 0x0814u: return { .lcid = lcid, .language = lcid_language::norwegian_nynorsk, .location = "Norway", .language_tag = "nn-NO", .release = lcid_release_key::release_a };
	case 0x0082u: return { .lcid = lcid, .language = lcid_language::occitan, .language_tag = "oc", .release = lcid_release_key::release_7 };
	case 0x0482u: return { .lcid = lcid, .language = lcid_language::occitan, .location = "France", .language_tag = "oc-FR", .release = lcid_release_key::release_v };
	case 0x0048u: return { .lcid = lcid, .language = lcid_language::odia, .language_tag = "or", .release = lcid_release_key::release_7 };
	case 0x0448u: return { .lcid = lcid, .language = lcid_language::odia, .location = "India", .language_tag = "or-IN", .release = lcid_release_key::release_v };
	case 0x0072u: return { .lcid = lcid, .language = lcid_language::oromo, .language_tag = "om", .release = lcid_release_key::release_8_1 };
	case 0x0472u: return { .lcid = lcid, .language = lcid_language::oromo, .location = "Ethiopia", .language_tag = "om-ET", .release = lcid_release_key::release_8_1 };
	case 0x0063u: return { .lcid = lcid, .language = lcid_language::pashto, .language_tag = "ps", .release = lcid_release_key::release_7 };
	case 0x0463u: return { .lcid = lcid, .language = lcid_language::pashto, .location = "Afghanistan", .language_tag = "ps-AF", .release = lcid_release_key::release_e2 };
	case 0x0029u: return { .lcid = lcid, .language = lcid_language::persian, .language_tag = "fa", .release = lcid_release_key::release_7 };
	case 0x0429u: return { .lcid = lcid, .language = lcid_language::persian, .location = "Iran", .language_tag = "fa-IR", .release = lcid_release_key::release_b };
	case 0x0015u: return { .lcid = lcid, .language = lcid_language::polish, .language_tag = "pl", .release = lcid_release_key::release_7 };
	case 0x0415u: return { .lcid = lcid, .language = lcid_language::polish, .location = "Poland", .language_tag = "pl-PL", .release = lcid_release_key::release_a };
	case 0x0016u: return { .lcid = lcid, .language = lcid_language::portuguese, .language_tag = "pt", .release = lcid_release_key::release_7 };
	case 0x0416u: return { .lcid = lcid, .language = lcid_language::portuguese, .location = "Brazil", .language_tag = "pt-BR", .release = lcid_release_key::release_a };
	case 0x0816u: return { .lcid = lcid, .language = lcid_language::portuguese, .location = "Portugal", .language_tag = "pt-PT", .release = lcid_release_key::release_a };
	case 0x05FEu: return { .lcid = lcid, .language = lcid_language::pseudo_language, .location = "Pseudo locale for east Asian/complex script localization testing", .language_tag = "qps-ploca", .release = lcid_release_key::release_7 };
	case 0x0501u: return { .lcid = lcid, .language = lcid_language::pseudo_language, .location = "Pseudo locale used for localization testing", .language_tag = "qps-ploc", .release = lcid_release_key::release_7 };
	case 0x09FFu: return { .lcid = lcid, .language = lcid_language::pseudo_language, .location = "Pseudo locale used for localization testing of mirrored locales", .language_tag = "qps-plocm", .release = lcid_release_key::release_7 };
	case 0x0046u: return { .lcid = lcid, .language = lcid_language::punjabi, .language_tag = "pa", .release = lcid_release_key::release_7 };
	case 0x7C46u: return { .lcid = lcid, .language = lcid_language::punjabi, .language_tag = "pa-Arab", .release = lcid_release_key::release_8 };
	case 0x0446u: return { .lcid = lcid, .language = lcid_language::punjabi, .location = "India", .language_tag = "pa-IN", .release = lcid_release_key::release_d };
	case 0x0846u: return { .lcid = lcid, .language = lcid_language::punjabi, .location = "Islamic Republic of Pakistan", .language_tag = "pa-Arab-PK", .release = lcid_release_key::release_8 };
	case 0x006Bu: return { .lcid = lcid, .language = lcid_language::quechua, .language_tag = "quz", .release = lcid_release_key::release_7 };
	case 0x046Bu: return { .lcid = lcid, .language = lcid_language::quechua, .location = "Bolivia", .language_tag = "quz-BO", .release = lcid_release_key::release_e1 };
	case 0x086Bu: return { .lcid = lcid, .language = lcid_language::quechua, .location = "Ecuador", .language_tag = "quz-EC", .release = lcid_release_key::release_e1 };
	case 0x0C6Bu: return { .lcid = lcid, .language = lcid_language::quechua, .location = "Peru", .language_tag = "quz-PE", .release = lcid_release_key::release_e1 };
	case 0x0018u: return { .lcid = lcid, .language = lcid_language::romanian, .language_tag = "ro", .release = lcid_release_key::release_7 };
	case 0x0818u: return { .lcid = lcid, .language = lcid_language::romanian, .location = "Moldova", .language_tag = "ro-MD", .release = lcid_release_key::release_8_1 };
	case 0x0418u: return { .lcid = lcid, .language = lcid_language::romanian, .location = "Romania", .language_tag = "ro-RO", .release = lcid_release_key::release_a };
	case 0x0017u: return { .lcid = lcid, .language = lcid_language::romansh, .language_tag = "rm", .release = lcid_release_key::release_7 };
	case 0x0417u: return { .lcid = lcid, .language = lcid_language::romansh, .location = "Switzerland", .language_tag = "rm-CH", .release = lcid_release_key::release_e2 };
	case 0x0019u: return { .lcid = lcid, .language = lcid_language::russian, .language_tag = "ru", .release = lcid_release_key::release_7 };
	case 0x0819u: return { .lcid = lcid, .language = lcid_language::russian, .location = "Moldova", .language_tag = "ru-MD", .release = lcid_release_key::release_10 };
	case 0x0419u: return { .lcid = lcid, .language = lcid_language::russian, .location = "Russia", .language_tag = "ru-RU", .release = lcid_release_key::release_a };
	case 0x0085u: return { .lcid = lcid, .language = lcid_language::sakha, .language_tag = "sah", .release = lcid_release_key::release_7 };
	case 0x0485u: return { .lcid = lcid, .language = lcid_language::sakha, .location = "Russia", .language_tag = "sah-RU", .release = lcid_release_key::release_v };
	case 0x703Bu: return { .lcid = lcid, .language = lcid_language::sami_inari, .language_tag = "smn", .release = lcid_release_key::release_7 };
	case 0x243Bu: return { .lcid = lcid, .language = lcid_language::sami_inari, .location = "Finland", .language_tag = "smn-FI", .release = lcid_release_key::release_e1 };
	case 0x7C3Bu: return { .lcid = lcid, .language = lcid_language::sami_lule, .language_tag = "smj", .release = lcid_release_key::release_7 };
	case 0x103Bu: return { .lcid = lcid, .language = lcid_language::sami_lule, .location = "Norway", .language_tag = "smj-NO", .release = lcid_release_key::release_e1 };
	case 0x143Bu: return { .lcid = lcid, .language = lcid_language::sami_lule, .location = "Sweden", .language_tag = "smj-SE", .release = lcid_release_key::release_e1 };
	case 0x003Bu: return { .lcid = lcid, .language = lcid_language::sami_northern, .language_tag = "se", .release = lcid_release_key::release_7 };
	case 0x0C3Bu: return { .lcid = lcid, .language = lcid_language::sami_northern, .location = "Finland", .language_tag = "se-FI", .release = lcid_release_key::release_e1 };
	case 0x043Bu: return { .lcid = lcid, .language = lcid_language::sami_northern, .location = "Norway", .language_tag = "se-NO", .release = lcid_release_key::release_e1 };
	case 0x083Bu: return { .lcid = lcid, .language = lcid_language::sami_northern, .location = "Sweden", .language_tag = "se-SE", .release = lcid_release_key::release_e1 };
	case 0x743Bu: return { .lcid = lcid, .language = lcid_language::sami_skolt, .language_tag = "sms", .release = lcid_release_key::release_7 };
	case 0x203Bu: return { .lcid = lcid, .language = lcid_language::sami_skolt, .location = "Finland", .language_tag = "sms-FI", .release = lcid_release_key::release_e1 };
	case 0x783Bu: return { .lcid = lcid, .language = lcid_language::sami_southern, .language_tag = "sma", .release = lcid_release_key::release_7 };
	case 0x183Bu: return { .lcid = lcid, .language = lcid_language::sami_southern, .location = "Norway", .language_tag = "sma-NO", .release = lcid_release_key::release_e1 };
	case 0x1C3Bu: return { .lcid = lcid, .language = lcid_language::sami_southern, .location = "Sweden", .language_tag = "sma-SE", .release = lcid_release_key::release_e1 };
	case 0x004Fu: return { .lcid = lcid, .language = lcid_language::sanskrit, .language_tag = "sa", .release = lcid_release_key::release_7 };
	case 0x044Fu: return { .lcid = lcid, .language = lcid_language::sanskrit, .location = "India", .language_tag = "sa-IN", .release = lcid_release_key::release_c };
	case 0x0091u: return { .lcid = lcid, .language = lcid_language::scottish_gaelic, .language_tag = "gd", .release = lcid_release_key::release_7 };
	case 0x0491u: return { .lcid = lcid, .language = lcid_language::scottish_gaelic, .location = "United Kingdom", .language_tag = "gd-GB", .release = lcid_release_key::release_7 };
	case 0x6C1Au: return { .lcid = lcid, .language = lcid_language::serbian_cyrillic, .language_tag = "sr-Cyrl", .release = lcid_release_key::release_7 };
	case 0x1C1Au: return { .lcid = lcid, .language = lcid_language::serbian_cyrillic, .location = "Bosnia and Herzegovina", .language_tag = "sr-Cyrl-BA", .release = lcid_release_key::release_e1 };
	case 0x301Au: return { .lcid = lcid, .language = lcid_language::serbian_cyrillic, .location = "Montenegro", .language_tag = "sr-Cyrl-ME", .release = lcid_release_key::release_7 };
	case 0x281Au: return { .lcid = lcid, .language = lcid_language::serbian_cyrillic, .location = "Serbia", .language_tag = "sr-Cyrl-RS", .release = lcid_release_key::release_7 };
	case 0x0C1Au: return { .lcid = lcid, .language = lcid_language::serbian_cyrillic, .location = "Serbia and Montenegro (Former)", .language_tag = "sr-Cyrl-CS", .release = lcid_release_key::release_b };
	case 0x701Au: return { .lcid = lcid, .language = lcid_language::serbian_latin, .language_tag = "sr-Latn", .release = lcid_release_key::release_7 };
	case 0x7C1Au: return { .lcid = lcid, .language = lcid_language::serbian_latin, .language_tag = "sr", .release = lcid_release_key::release_7 };
	case 0x181Au: return { .lcid = lcid, .language = lcid_language::serbian_latin, .location = "Bosnia and Herzegovina", .language_tag = "sr-Latn-BA", .release = lcid_release_key::release_e1 };
	case 0x2c1Au: return { .lcid = lcid, .language = lcid_language::serbian_latin, .location = "Montenegro", .language_tag = "sr-Latn-ME", .release = lcid_release_key::release_7 };
	case 0x241Au: return { .lcid = lcid, .language = lcid_language::serbian_latin, .location = "Serbia", .language_tag = "sr-Latn-RS", .release = lcid_release_key::release_7 };
	case 0x081Au: return { .lcid = lcid, .language = lcid_language::serbian_latin, .location = "Serbia and Montenegro (Former)", .language_tag = "sr-Latn-CS", .release = lcid_release_key::release_b };
	case 0x006Cu: return { .lcid = lcid, .language = lcid_language::sesotho_sa_leboa, .language_tag = "nso", .release = lcid_release_key::release_7 };
	case 0x046Cu: return { .lcid = lcid, .language = lcid_language::sesotho_sa_leboa, .location = "South Africa", .language_tag = "nso-ZA", .release = lcid_release_key::release_e1 };
	case 0x0032u: return { .lcid = lcid, .language = lcid_language::setswana, .language_tag = "tn", .release = lcid_release_key::release_7 };
	case 0x0832u: return { .lcid = lcid, .language = lcid_language::setswana, .location = "Botswana", .language_tag = "tn-BW", .release = lcid_release_key::release_8 };
	case 0x0432u: return { .lcid = lcid, .language = lcid_language::setswana, .location = "South Africa", .language_tag = "tn-ZA", .release = lcid_release_key::release_e1 };
	case 0x0059u: return { .lcid = lcid, .language = lcid_language::sindhi, .language_tag = "sd", .release = lcid_release_key::release_8 };
	case 0x7C59u: return { .lcid = lcid, .language = lcid_language::sindhi, .language_tag = "sd-Arab", .release = lcid_release_key::release_8 };
	case 0x0859u: return { .lcid = lcid, .language = lcid_language::sindhi, .location = "Islamic Republic of Pakistan", .language_tag = "sd-Arab-PK", .release = lcid_release_key::release_8 };
	case 0x005Bu: return { .lcid = lcid, .language = lcid_language::sinhala, .language_tag = "si", .release = lcid_release_key::release_7 };
	case 0x045Bu: return { .lcid = lcid, .language = lcid_language::sinhala, .location = "Sri Lanka", .language_tag = "si-LK", .release = lcid_release_key::release_v };
	case 0x001Bu: return { .lcid = lcid, .language = lcid_language::slovak, .language_tag = "sk", .release = lcid_release_key::release_7 };
	case 0x041Bu: return { .lcid = lcid, .language = lcid_language::slovak, .location = "Slovakia", .language_tag = "sk-SK", .release = lcid_release_key::release_a };
	case 0x0024u: return { .lcid = lcid, .language = lcid_language::slovenian, .language_tag = "sl", .release = lcid_release_key::release_7 };
	case 0x0424u: return { .lcid = lcid, .language = lcid_language::slovenian, .location = "Slovenia", .language_tag = "sl-SI", .release = lcid_release_key::release_a };
	case 0x0077u: return { .lcid = lcid, .language = lcid_language::somali, .language_tag = "so", .release = lcid_release_key::release_8_1 };
	case 0x0477u: return { .lcid = lcid, .language = lcid_language::somali, .location = "Somalia", .language_tag = "so-SO", .release = lcid_release_key::release_8_1 };
	case 0x0030u: return { .lcid = lcid, .language = lcid_language::sotho, .language_tag = "st", .release = lcid_release_key::release_8_1 };
	case 0x0430u: return { .lcid = lcid, .language = lcid_language::sotho, .location = "South Africa", .language_tag = "st-ZA", .release = lcid_release_key::release_8_1 };
	case 0x000Au: return { .lcid = lcid, .language = lcid_language::spanish, .language_tag = "es", .release = lcid_release_key::release_7 };
	case 0x2C0Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Argentina", .language_tag = "es-AR", .release = lcid_release_key::release_b };
	case 0x200Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Bolivarian Republic of Venezuela", .language_tag = "es-VE", .release = lcid_release_key::release_b };
	case 0x400Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Bolivia", .language_tag = "es-BO", .release = lcid_release_key::release_b };
	case 0x340Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Chile", .language_tag = "es-CL", .release = lcid_release_key::release_b };
	case 0x240Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Colombia", .language_tag = "es-CO", .release = lcid_release_key::release_b };
	case 0x140Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Costa Rica", .language_tag = "es-CR", .release = lcid_release_key::release_b };
	case 0x5c0Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Cuba", .language_tag = "es-CU", .release = lcid_release_key::release_10 };
	case 0x1c0Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Dominican Republic", .language_tag = "es-DO", .release = lcid_release_key::release_b };
	case 0x300Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Ecuador", .language_tag = "es-EC", .release = lcid_release_key::release_b };
	case 0x440Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "El Salvador", .language_tag = "es-SV", .release = lcid_release_key::release_b };
	case 0x100Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Guatemala", .language_tag = "es-GT", .release = lcid_release_key::release_b };
	case 0x480Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Honduras", .language_tag = "es-HN", .release = lcid_release_key::release_b };
	case 0x580Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Latin America", .language_tag = "es-419", .release = lcid_release_key::release_8_1 };
	case 0x080Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Mexico", .language_tag = "es-MX", .release = lcid_release_key::release_a };
	case 0x4C0Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Nicaragua", .language_tag = "es-NI", .release = lcid_release_key::release_b };
	case 0x180Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Panama", .language_tag = "es-PA", .release = lcid_release_key::release_b };
	case 0x3C0Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Paraguay", .language_tag = "es-PY", .release = lcid_release_key::release_b };
	case 0x280Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Peru", .language_tag = "es-PE", .release = lcid_release_key::release_b };
	case 0x500Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Puerto Rico", .language_tag = "es-PR", .release = lcid_release_key::release_b };
	case 0x040Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Spain", .language_tag = "es-ES_tradnl", .release = lcid_release_key::release_a };
	case 0x0c0Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Spain", .language_tag = "es-ES", .release = lcid_release_key::release_a };
	case 0x540Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "United States", .language_tag = "es-US", .release = lcid_release_key::release_v };
	case 0x380Au: return { .lcid = lcid, .language = lcid_language::spanish, .location = "Uruguay", .language_tag = "es-UY", .release = lcid_release_key::release_b };
	case 0x001Du: return { .lcid = lcid, .language = lcid_language::swedish, .language_tag = "sv", .release = lcid_release_key::release_7 };
	case 0x081Du: return { .lcid = lcid, .language = lcid_language::swedish, .location = "Finland", .language_tag = "sv-FI", .release = lcid_release_key::release_b };
	case 0x041Du: return { .lcid = lcid, .language = lcid_language::swedish, .location = "Sweden", .language_tag = "sv-SE", .release = lcid_release_key::release_a };
	case 0x005Au: return { .lcid = lcid, .language = lcid_language::syriac, .language_tag = "syr", .release = lcid_release_key::release_7 };
	case 0x045Au: return { .lcid = lcid, .language = lcid_language::syriac, .location = "Syria", .language_tag = "syr-SY", .release = lcid_release_key::release_d };
	case 0x0028u: return { .lcid = lcid, .language = lcid_language::tajik_cyrillic, .language_tag = "tg", .release = lcid_release_key::release_7 };
	case 0x7C28u: return { .lcid = lcid, .language = lcid_language::tajik_cyrillic, .language_tag = "tg-Cyrl", .release = lcid_release_key::release_7 };
	case 0x0428u: return { .lcid = lcid, .language = lcid_language::tajik_cyrillic, .location = "Tajikistan", .language_tag = "tg-Cyrl-TJ", .release = lcid_release_key::release_v };
	case 0x005Fu: return { .lcid = lcid, .language = lcid_language::tamazight_latin, .language_tag = "tzm", .release = lcid_release_key::release_7 };
	case 0x7C5Fu: return { .lcid = lcid, .language = lcid_language::tamazight_latin, .language_tag = "tzm-Latn", .release = lcid_release_key::release_7 };
	case 0x085Fu: return { .lcid = lcid, .language = lcid_language::tamazight_latin, .location = "Algeria", .language_tag = "tzm-Latn-DZ", .release = lcid_release_key::release_v };
	case 0x0049u: return { .lcid = lcid, .language = lcid_language::tamil, .language_tag = "ta", .release = lcid_release_key::release_7 };
	case 0x0449u: return { .lcid = lcid, .language = lcid_language::tamil, .location = "India", .language_tag = "ta-IN", .release = lcid_release_key::release_c };
	case 0x0849u: return { .lcid = lcid, .language = lcid_language::tamil, .location = "Sri Lanka", .language_tag = "ta-LK", .release = lcid_release_key::release_8 };
	case 0x0044u: return { .lcid = lcid, .language = lcid_language::tatar, .language_tag = "tt", .release = lcid_release_key::release_7 };
	case 0x0444u: return { .lcid = lcid, .language = lcid_language::tatar, .location = "Russia", .language_tag = "tt-RU", .release = lcid_release_key::release_d };
	case 0x004Au: return { .lcid = lcid, .language = lcid_language::telugu, .language_tag = "te", .release = lcid_release_key::release_7 };
	case 0x044Au: return { .lcid = lcid, .language = lcid_language::telugu, .location = "India", .language_tag = "te-IN", .release = lcid_release_key::release_d };
	case 0x001Eu: return { .lcid = lcid, .language = lcid_language::thai, .language_tag = "th", .release = lcid_release_key::release_7 };
	case 0x041Eu: return { .lcid = lcid, .language = lcid_language::thai, .location = "Thailand", .language_tag = "th-TH", .release = lcid_release_key::release_b };
	case 0x0051u: return { .lcid = lcid, .language = lcid_language::tibetan, .language_tag = "bo", .release = lcid_release_key::release_7 };
	case 0x0451u: return { .lcid = lcid, .language = lcid_language::tibetan, .location = "People's Republic of China", .language_tag = "bo-CN", .release = lcid_release_key::release_v };
	case 0x0073u: return { .lcid = lcid, .language = lcid_language::tigrinya, .language_tag = "ti", .release = lcid_release_key::release_8 };
	case 0x0873u: return { .lcid = lcid, .language = lcid_language::tigrinya, .location = "Eritrea", .language_tag = "ti-ER", .release = lcid_release_key::release_8 };
	case 0x0473u: return { .lcid = lcid, .language = lcid_language::tigrinya, .location = "Ethiopia", .language_tag = "ti-ET", .release = lcid_release_key::release_8 };
	case 0x0031u: return { .lcid = lcid, .language = lcid_language::tsonga, .language_tag = "ts", .release = lcid_release_key::release_8_1 };
	case 0x0431u: return { .lcid = lcid, .language = lcid_language::tsonga, .location = "South Africa", .language_tag = "ts-ZA", .release = lcid_release_key::release_8_1 };
	case 0x001Fu: return { .lcid = lcid, .language = lcid_language::turkish, .language_tag = "tr", .release = lcid_release_key::release_7 };
	case 0x041Fu: return { .lcid = lcid, .language = lcid_language::turkish, .location = "Turkey", .language_tag = "tr-TR", .release = lcid_release_key::release_a };
	case 0x0042u: return { .lcid = lcid, .language = lcid_language::turkmen, .language_tag = "tk", .release = lcid_release_key::release_7 };
	case 0x0442u: return { .lcid = lcid, .language = lcid_language::turkmen, .location = "Turkmenistan", .language_tag = "tk-TM", .release = lcid_release_key::release_v };
	case 0x0022u: return { .lcid = lcid, .language = lcid_language::ukrainian, .language_tag = "uk", .release = lcid_release_key::release_7 };
	case 0x0422u: return { .lcid = lcid, .language = lcid_language::ukrainian, .location = "Ukraine", .language_tag = "uk-UA", .release = lcid_release_key::release_b };
	case 0x002Eu: return { .lcid = lcid, .language = lcid_language::upper_sorbian, .language_tag = "hsb", .release = lcid_release_key::release_7 };
	case 0x042Eu: return { .lcid = lcid, .language = lcid_language::upper_sorbian, .location = "Germany", .language_tag = "hsb-DE", .release = lcid_release_key::release_v };
	case 0x0020u: return { .lcid = lcid, .language = lcid_language::urdu, .language_tag = "ur", .release = lcid_release_key::release_7 };
	case 0x0820u: return { .lcid = lcid, .language = lcid_language::urdu, .location = "India", .language_tag = "ur-IN", .release = lcid_release_key::release_8_1 };
	case 0x0420u: return { .lcid = lcid, .language = lcid_language::urdu, .location = "Islamic Republic of Pakistan", .language_tag = "ur-PK", .release = lcid_release_key::release_c };
	case 0x0080u: return { .lcid = lcid, .language = lcid_language::uyghur, .language_tag = "ug", .release = lcid_release_key::release_7 };
	case 0x0480u: return { .lcid = lcid, .language = lcid_language::uyghur, .location = "People's Republic of China", .language_tag = "ug-CN", .release = lcid_release_key::release_v };
	case 0x7843u: return { .lcid = lcid, .language = lcid_language::uzbek_cyrillic, .language_tag = "uz-Cyrl", .release = lcid_release_key::release_7 };
	case 0x0843u: return { .lcid = lcid, .language = lcid_language::uzbek_cyrillic, .location = "Uzbekistan", .language_tag = "uz-Cyrl-UZ", .release = lcid_release_key::release_c };
	case 0x0043u: return { .lcid = lcid, .language = lcid_language::uzbek_latin, .language_tag = "uz", .release = lcid_release_key::release_7 };
	case 0x7C43u: return { .lcid = lcid, .language = lcid_language::uzbek_latin, .language_tag = "uz-Latn", .release = lcid_release_key::release_7 };
	case 0x0443u: return { .lcid = lcid, .language = lcid_language::uzbek_latin, .location = "Uzbekistan", .language_tag = "uz-Latn-UZ", .release = lcid_release_key::release_c };
	case 0x0803u: return { .lcid = lcid, .language = lcid_language::valencian, .location = "Spain", .language_tag = "ca-ESvalencia", .release = lcid_release_key::release_8 };
	case 0x0033u: return { .lcid = lcid, .language = lcid_language::venda, .language_tag = "ve", .release = lcid_release_key::release_10 };
	case 0x0433u: return { .lcid = lcid, .language = lcid_language::venda, .location = "South Africa", .language_tag = "ve-ZA", .release = lcid_release_key::release_10 };
	case 0x002Au: return { .lcid = lcid, .language = lcid_language::vietnamese, .language_tag = "vi", .release = lcid_release_key::release_7 };
	case 0x042Au: return { .lcid = lcid, .language = lcid_language::vietnamese, .location = "Vietnam", .language_tag = "vi-VN", .release = lcid_release_key::release_b };
	case 0x0052u: return { .lcid = lcid, .language = lcid_language::welsh, .language_tag = "cy", .release = lcid_release_key::release_7 };
	case 0x0452u: return { .lcid = lcid, .language = lcid_language::welsh, .location = "United Kingdom", .language_tag = "cy-GB", .release = lcid_release_key::release_e1 };
	case 0x0088u: return { .lcid = lcid, .language = lcid_language::wolof, .language_tag = "wo", .release = lcid_release_key::release_7 };
	case 0x0488u: return { .lcid = lcid, .language = lcid_language::wolof, .location = "Senegal", .language_tag = "wo-SN", .release = lcid_release_key::release_v };
	case 0x0034u: return { .lcid = lcid, .language = lcid_language::xhosa, .language_tag = "xh", .release = lcid_release_key::release_7 };
	case 0x0434u: return { .lcid = lcid, .language = lcid_language::xhosa, .location = "South Africa", .language_tag = "xh-ZA", .release = lcid_release_key::release_e1 };
	case 0x0078u: return { .lcid = lcid, .language = lcid_language::yi, .language_tag = "ii", .release = lcid_release_key::release_7 };
	case 0x0478u: return { .lcid = lcid, .language = lcid_language::yi, .location = "People's Republic of China", .language_tag = "ii-CN", .release = lcid_release_key::release_v };
	case 0x043Du: return { .lcid = lcid, .language = lcid_language::yiddish, .location = "World", .language_tag = "yi-001", .release = lcid_release_key::release_10 };
	case 0x006Au: return { .lcid = lcid, .language = lcid_language::yoruba, .language_tag = "yo", .release = lcid_release_key::release_7 };
	case 0x046Au: return { .lcid = lcid, .language = lcid_language::yoruba, .location = "Nigeria", .language_tag = "yo-NG", .release = lcid_release_key::release_v };
	case 0x0035u: return { .lcid = lcid, .language = lcid_language::zulu, .language_tag = "zu", .release = lcid_release_key::release_7 };
	case 0x0435u: return { .lcid = lcid, .language = lcid_language::zulu, .location = "South Africa", .language_tag = "zu-ZA", .release = lcid_release_key::release_e1 };
	case 0x0000u: return { .lcid = lcid, .language = lcid_language::neutral, .language_tag = "", .release = lcid_release_key::release_a };
	case 0x0400u: return { .lcid = lcid, .language = lcid_language::process_default, .language_tag = "", .release = lcid_release_key::release_a };
	case 0x0800u: return { .lcid = lcid, .language = lcid_language::system_default, .language_tag = "", .release = lcid_release_key::release_a };
	default: return {};
	}
}
} //namespace

std::optional<lcid_info> get_lcid_info(lcid_type lcid) noexcept
{
	auto info = get_lcid_info_impl(lcid);
	if (static_cast<std::uint32_t>(info.language))
		return info;
	return {};
}

std::optional<lcid_info> get_lcid_info_impl(std::string_view lang_code) noexcept
{
	static const std::unordered_map<std::string_view, lcid_type> lang_to_lcid{
		{ "af", 0x0036u },
		{ "af-za", 0x0436u },
		{ "sq", 0x001cu },
		{ "sq-al", 0x041cu },
		{ "gsw", 0x0084u },
		{ "gsw-fr", 0x0484u },
		{ "am", 0x005eu },
		{ "am-et", 0x045eu },
		{ "ar", 0x0001u },
		{ "ar-dz", 0x1401u },
		{ "ar-bh", 0x3c01u },
		{ "ar-eg", 0x0c01u },
		{ "ar-iq", 0x0801u },
		{ "ar-jo", 0x2c01u },
		{ "ar-kw", 0x3401u },
		{ "ar-lb", 0x3001u },
		{ "ar-ly", 0x1001u },
		{ "ar-ma", 0x1801u },
		{ "ar-om", 0x2001u },
		{ "ar-qa", 0x4001u },
		{ "ar-sa", 0x0401u },
		{ "ar-sy", 0x2801u },
		{ "ar-tn", 0x1c01u },
		{ "ar-ae", 0x3801u },
		{ "ar-ye", 0x2401u },
		{ "hy", 0x002bu },
		{ "hy-am", 0x042bu },
		{ "as", 0x004du },
		{ "as-in", 0x044du },
		{ "az-cyrl", 0x742cu },
		{ "az-cyrl-az", 0x082cu },
		{ "az", 0x002cu },
		{ "az-latn", 0x782cu },
		{ "az-latn-az", 0x042cu },
		{ "bn", 0x0045u },
		{ "bn-bd", 0x0845u },
		{ "bn-in", 0x0445u },
		{ "ba", 0x006du },
		{ "ba-ru", 0x046du },
		{ "eu", 0x002du },
		{ "eu-es", 0x042du },
		{ "be", 0x0023u },
		{ "be-by", 0x0423u },
		{ "bs-cyrl", 0x641au },
		{ "bs-cyrl-ba", 0x201au },
		{ "bs-latn", 0x681au },
		{ "bs", 0x781au },
		{ "bs-latn-ba", 0x141au },
		{ "br", 0x007eu },
		{ "br-fr", 0x047eu },
		{ "bg", 0x0002u },
		{ "bg-bg", 0x0402u },
		{ "my", 0x0055u },
		{ "my-mm", 0x0455u },
		{ "ca", 0x0003u },
		{ "ca-es", 0x0403u },
		{ "tzm-arabma", 0x045fu },
		{ "ku", 0x0092u },
		{ "ku-arab", 0x7c92u },
		{ "ku-arab-iq", 0x0492u },
		{ "chr", 0x005cu },
		{ "chr-cher", 0x7c5cu },
		{ "chr-cher-us", 0x045cu },
		{ "zh-hans", 0x0004u },
		{ "zh", 0x7804u },
		{ "zh-cn", 0x0804u },
		{ "zh-sg", 0x1004u },
		{ "zh-hant", 0x7c04u },
		{ "zh-hk", 0x0c04u },
		{ "zh-mo", 0x1404u },
		{ "zh-tw", 0x0404u },
		{ "co", 0x0083u },
		{ "co-fr", 0x0483u },
		{ "hr", 0x001au },
		{ "hr-hr", 0x041au },
		{ "hr-ba", 0x101au },
		{ "cs", 0x0005u },
		{ "cs-cz", 0x0405u },
		{ "da", 0x0006u },
		{ "da-dk", 0x0406u },
		{ "prs", 0x008cu },
		{ "prs-af", 0x048cu },
		{ "dv", 0x0065u },
		{ "dv-mv", 0x0465u },
		{ "nl", 0x0013u },
		{ "nl-be", 0x0813u },
		{ "nl-nl", 0x0413u },
		{ "dz-bt", 0x0c51u },
		{ "en", 0x0009u },
		{ "en-au", 0x0c09u },
		{ "en-bz", 0x2809u },
		{ "en-ca", 0x1009u },
		{ "en-029", 0x2409u },
		{ "en-hk", 0x3c09u },
		{ "en-in", 0x4009u },
		{ "en-ie", 0x1809u },
		{ "en-jm", 0x2009u },
		{ "en-my", 0x4409u },
		{ "en-nz", 0x1409u },
		{ "en-ph", 0x3409u },
		{ "en-sg", 0x4809u },
		{ "en-za", 0x1c09u },
		{ "en-tt", 0x2c09u },
		{ "en-ae", 0x4c09u },
		{ "en-gb", 0x0809u },
		{ "en-us", 0x0409u },
		{ "en-zw", 0x3009u },
		{ "et", 0x0025u },
		{ "et-ee", 0x0425u },
		{ "fo", 0x0038u },
		{ "fo-fo", 0x0438u },
		{ "fil", 0x0064u },
		{ "fil-ph", 0x0464u },
		{ "fi", 0x000bu },
		{ "fi-fi", 0x040bu },
		{ "fr", 0x000cu },
		{ "fr-be", 0x080cu },
		{ "fr-cm", 0x2c0cu },
		{ "fr-ca", 0x0c0cu },
		{ "fr-029", 0x1c0cu },
		{ "fr-cd", 0x240cu },
		{ "fr-ci", 0x300cu },
		{ "fr-fr", 0x040cu },
		{ "fr-ht", 0x3c0cu },
		{ "fr-lu", 0x140cu },
		{ "fr-ml", 0x340cu },
		{ "fr-ma", 0x380cu },
		{ "fr-mc", 0x180cu },
		{ "fr-re", 0x200cu },
		{ "fr-sn", 0x280cu },
		{ "fr-ch", 0x100cu },
		{ "fy", 0x0062u },
		{ "fy-nl", 0x0462u },
		{ "ff", 0x0067u },
		{ "ff-latn", 0x7c67u },
		{ "ff-ng", 0x0467u },
		{ "ff-latn-sn", 0x0867u },
		{ "gl", 0x0056u },
		{ "gl-es", 0x0456u },
		{ "ka", 0x0037u },
		{ "ka-ge", 0x0437u },
		{ "de", 0x0007u },
		{ "de-at", 0x0c07u },
		{ "de-de", 0x0407u },
		{ "de-li", 0x1407u },
		{ "de-lu", 0x1007u },
		{ "de-ch", 0x0807u },
		{ "el", 0x0008u },
		{ "el-gr", 0x0408u },
		{ "kl", 0x006fu },
		{ "kl-gl", 0x046fu },
		{ "gn", 0x0074u },
		{ "gn-py", 0x0474u },
		{ "gu", 0x0047u },
		{ "gu-in", 0x0447u },
		{ "ha", 0x0068u },
		{ "ha-latn", 0x7c68u },
		{ "ha-latn-ng", 0x0468u },
		{ "haw", 0x0075u },
		{ "haw-us", 0x0475u },
		{ "he", 0x000du },
		{ "he-il", 0x040du },
		{ "hi", 0x0039u },
		{ "hi-in", 0x0439u },
		{ "hu", 0x000eu },
		{ "hu-hu", 0x040eu },
		{ "is", 0x000fu },
		{ "is-is", 0x040fu },
		{ "ig", 0x0070u },
		{ "ig-ng", 0x0470u },
		{ "id", 0x0021u },
		{ "id-id", 0x0421u },
		{ "iu", 0x005du },
		{ "iu-latn", 0x7c5du },
		{ "iu-latn-ca", 0x085du },
		{ "iu-cans", 0x785du },
		{ "iu-cans-ca", 0x045du },
		{ "ga", 0x003cu },
		{ "ga-ie", 0x083cu },
		{ "it", 0x0010u },
		{ "it-it", 0x0410u },
		{ "it-ch", 0x0810u },
		{ "ja", 0x0011u },
		{ "ja-jp", 0x0411u },
		{ "kn", 0x004bu },
		{ "kn-in", 0x044bu },
		{ "kr-latn-ng", 0x0471u },
		{ "ks", 0x0060u },
		{ "ks-arab", 0x0460u },
		{ "ks-deva-in", 0x0860u },
		{ "kk", 0x003fu },
		{ "kk-kz", 0x043fu },
		{ "km", 0x0053u },
		{ "km-kh", 0x0453u },
		{ "quc", 0x0086u },
		{ "quc-latn-gt", 0x0486u },
		{ "rw", 0x0087u },
		{ "rw-rw", 0x0487u },
		{ "sw", 0x0041u },
		{ "sw-ke", 0x0441u },
		{ "kok", 0x0057u },
		{ "kok-in", 0x0457u },
		{ "ko", 0x0012u },
		{ "ko-kr", 0x0412u },
		{ "ky", 0x0040u },
		{ "ky-kg", 0x0440u },
		{ "lo", 0x0054u },
		{ "lo-la", 0x0454u },
		{ "la-va", 0x0476u },
		{ "lv", 0x0026u },
		{ "lv-lv", 0x0426u },
		{ "lt", 0x0027u },
		{ "lt-lt", 0x0427u },
		{ "dsb", 0x7c2eu },
		{ "dsb-de", 0x082eu },
		{ "lb", 0x006eu },
		{ "lb-lu", 0x046eu },
		{ "mk", 0x002fu },
		{ "mk-mk", 0x042fu },
		{ "ms", 0x003eu },
		{ "ms-bn", 0x083eu },
		{ "ms-my", 0x043eu },
		{ "ml", 0x004cu },
		{ "ml-in", 0x044cu },
		{ "mt", 0x003au },
		{ "mt-mt", 0x043au },
		{ "mi", 0x0081u },
		{ "mi-nz", 0x0481u },
		{ "arn", 0x007au },
		{ "arn-cl", 0x047au },
		{ "mr", 0x004eu },
		{ "mr-in", 0x044eu },
		{ "moh", 0x007cu },
		{ "moh-ca", 0x047cu },
		{ "mn", 0x0050u },
		{ "mn-cyrl", 0x7850u },
		{ "mn-mn", 0x0450u },
		{ "mn-mong", 0x7c50u },
		{ "mn-mongcn", 0x0850u },
		{ "mn-mongmn", 0x0c50u },
		{ "ne", 0x0061u },
		{ "ne-in", 0x0861u },
		{ "ne-np", 0x0461u },
		{ "no", 0x0014u },
		{ "nb", 0x7c14u },
		{ "nb-no", 0x0414u },
		{ "nn", 0x7814u },
		{ "nn-no", 0x0814u },
		{ "oc", 0x0082u },
		{ "oc-fr", 0x0482u },
		{ "or", 0x0048u },
		{ "or-in", 0x0448u },
		{ "om", 0x0072u },
		{ "om-et", 0x0472u },
		{ "ps", 0x0063u },
		{ "ps-af", 0x0463u },
		{ "fa", 0x0029u },
		{ "fa-ir", 0x0429u },
		{ "pl", 0x0015u },
		{ "pl-pl", 0x0415u },
		{ "pt", 0x0016u },
		{ "pt-br", 0x0416u },
		{ "pt-pt", 0x0816u },
		{ "qps-ploca", 0x05feu },
		{ "qps-ploc", 0x0501u },
		{ "qps-plocm", 0x09ffu },
		{ "pa", 0x0046u },
		{ "pa-arab", 0x7c46u },
		{ "pa-in", 0x0446u },
		{ "pa-arab-pk", 0x0846u },
		{ "quz", 0x006bu },
		{ "quz-bo", 0x046bu },
		{ "quz-ec", 0x086bu },
		{ "quz-pe", 0x0c6bu },
		{ "ro", 0x0018u },
		{ "ro-md", 0x0818u },
		{ "ro-ro", 0x0418u },
		{ "rm", 0x0017u },
		{ "rm-ch", 0x0417u },
		{ "ru", 0x0019u },
		{ "ru-md", 0x0819u },
		{ "ru-ru", 0x0419u },
		{ "sah", 0x0085u },
		{ "sah-ru", 0x0485u },
		{ "smn", 0x703bu },
		{ "smn-fi", 0x243bu },
		{ "smj", 0x7c3bu },
		{ "smj-no", 0x103bu },
		{ "smj-se", 0x143bu },
		{ "se", 0x003bu },
		{ "se-fi", 0x0c3bu },
		{ "se-no", 0x043bu },
		{ "se-se", 0x083bu },
		{ "sms", 0x743bu },
		{ "sms-fi", 0x203bu },
		{ "sma", 0x783bu },
		{ "sma-no", 0x183bu },
		{ "sma-se", 0x1c3bu },
		{ "sa", 0x004fu },
		{ "sa-in", 0x044fu },
		{ "gd", 0x0091u },
		{ "gd-gb", 0x0491u },
		{ "sr-cyrl", 0x6c1au },
		{ "sr-cyrl-ba", 0x1c1au },
		{ "sr-cyrl-me", 0x301au },
		{ "sr-cyrl-rs", 0x281au },
		{ "sr-cyrl-cs", 0x0c1au },
		{ "sr-latn", 0x701au },
		{ "sr", 0x7c1au },
		{ "sr-latn-ba", 0x181au },
		{ "sr-latn-me", 0x2c1au },
		{ "sr-latn-rs", 0x241au },
		{ "sr-latn-cs", 0x081au },
		{ "nso", 0x006cu },
		{ "nso-za", 0x046cu },
		{ "tn", 0x0032u },
		{ "tn-bw", 0x0832u },
		{ "tn-za", 0x0432u },
		{ "sd", 0x0059u },
		{ "sd-arab", 0x7c59u },
		{ "sd-arab-pk", 0x0859u },
		{ "si", 0x005bu },
		{ "si-lk", 0x045bu },
		{ "sk", 0x001bu },
		{ "sk-sk", 0x041bu },
		{ "sl", 0x0024u },
		{ "sl-si", 0x0424u },
		{ "so", 0x0077u },
		{ "so-so", 0x0477u },
		{ "st", 0x0030u },
		{ "st-za", 0x0430u },
		{ "es", 0x000au },
		{ "es-ar", 0x2c0au },
		{ "es-ve", 0x200au },
		{ "es-bo", 0x400au },
		{ "es-cl", 0x340au },
		{ "es-co", 0x240au },
		{ "es-cr", 0x140au },
		{ "es-cu", 0x5c0au },
		{ "es-do", 0x1c0au },
		{ "es-ec", 0x300au },
		{ "es-sv", 0x440au },
		{ "es-gt", 0x100au },
		{ "es-hn", 0x480au },
		{ "es-419", 0x580au },
		{ "es-mx", 0x080au },
		{ "es-ni", 0x4c0au },
		{ "es-pa", 0x180au },
		{ "es-py", 0x3c0au },
		{ "es-pe", 0x280au },
		{ "es-pr", 0x500au },
		{ "es-es_tradnl", 0x040au },
		{ "es-es", 0x0c0au },
		{ "es-us", 0x540au },
		{ "es-uy", 0x380au },
		{ "sv", 0x001du },
		{ "sv-fi", 0x081du },
		{ "sv-se", 0x041du },
		{ "syr", 0x005au },
		{ "syr-sy", 0x045au },
		{ "tg", 0x0028u },
		{ "tg-cyrl", 0x7c28u },
		{ "tg-cyrl-tj", 0x0428u },
		{ "tzm", 0x005fu },
		{ "tzm-latn", 0x7c5fu },
		{ "tzm-latn-dz", 0x085fu },
		{ "ta", 0x0049u },
		{ "ta-in", 0x0449u },
		{ "ta-lk", 0x0849u },
		{ "tt", 0x0044u },
		{ "tt-ru", 0x0444u },
		{ "te", 0x004au },
		{ "te-in", 0x044au },
		{ "th", 0x001eu },
		{ "th-th", 0x041eu },
		{ "bo", 0x0051u },
		{ "bo-cn", 0x0451u },
		{ "ti", 0x0073u },
		{ "ti-er", 0x0873u },
		{ "ti-et", 0x0473u },
		{ "ts", 0x0031u },
		{ "ts-za", 0x0431u },
		{ "tr", 0x001fu },
		{ "tr-tr", 0x041fu },
		{ "tk", 0x0042u },
		{ "tk-tm", 0x0442u },
		{ "uk", 0x0022u },
		{ "uk-ua", 0x0422u },
		{ "hsb", 0x002eu },
		{ "hsb-de", 0x042eu },
		{ "ur", 0x0020u },
		{ "ur-in", 0x0820u },
		{ "ur-pk", 0x0420u },
		{ "ug", 0x0080u },
		{ "ug-cn", 0x0480u },
		{ "uz-cyrl", 0x7843u },
		{ "uz-cyrl-uz", 0x0843u },
		{ "uz", 0x0043u },
		{ "uz-latn", 0x7c43u },
		{ "uz-latn-uz", 0x0443u },
		{ "ca-esvalencia", 0x0803u },
		{ "ve", 0x0033u },
		{ "ve-za", 0x0433u },
		{ "vi", 0x002au },
		{ "vi-vn", 0x042au },
		{ "cy", 0x0052u },
		{ "cy-gb", 0x0452u },
		{ "wo", 0x0088u },
		{ "wo-sn", 0x0488u },
		{ "xh", 0x0034u },
		{ "xh-za", 0x0434u },
		{ "ii", 0x0078u },
		{ "ii-cn", 0x0478u },
		{ "yi-001", 0x043du },
		{ "yo", 0x006au },
		{ "yo-ng", 0x046au },
		{ "zu", 0x0035u },
		{ "zu-za", 0x0435u }
	};

	auto it = lang_to_lcid.find(lang_code);
	if (it == lang_to_lcid.cend())
		return {};

	return get_lcid_info(it->second);
}

} //namespace pe_bliss::resources
