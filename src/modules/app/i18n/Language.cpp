/**
 * @file
 */

// tinygettext - A gettext replacement that works directly on .po files
// Copyright (c) 2006 Ingo Ruhnke <grumbel@gmail.com>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgement in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include "Language.h"
#include "core/String.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicStringMap.h"

namespace app {

struct LanguageSpec {
	/** Language code: "de", "en", ... */
	const char *language;

	/** Country code: "BR", "DE", ..., can be 0 */
	const char *country;

	/** Modifier/Varint: "Latn", "ije", "latin"..., can be 0 */
	const char *modifier;

	/** Language name: "German", "English", "French", ... */
	const char *name;

	/** Language name in the specified language */
	const char *name_localized;
};

/** Language Definitions */
//*{
static const LanguageSpec languages[] = {{"aa", 0, 0, "Afar", "ʿAfár af"},
										 {"af", 0, 0, "Afrikaans", "Afrikaans"},
										 {"af", "ZA", 0, "Afrikaans (South Africa)", 0},
										 {"am", 0, 0, "Amharic", "ኣማርኛ"},
										 {"ar", 0, 0, "Arabic", "العربية"},
										 {"ar", "AR", 0, "Arabic (Argentina)", 0},
										 {"ar", "OM", 0, "Arabic (Oman)", 0},
										 {"ar", "SA", 0, "Arabic (Saudi Arabia)", 0},
										 {"ar", "SY", 0, "Arabic (Syrian Arab Republic)", 0},
										 {"ar", "TN", 0, "Arabic (Tunisia)", 0},
										 {"as", 0, 0, "Assamese", "অসমীয়া"},
										 {"ast", 0, 0, "Asturian", "Asturianu"},
										 {"ay", 0, 0, "Aymara", "aymar aru"},
										 {"az", 0, 0, "Azerbaijani", "Azərbaycanca"},
										 {"az", "IR", 0, "Azerbaijani (Iran)", 0},
										 {"be", 0, 0, "Belarusian", "Беларуская мова"},
										 {"be", 0, "latin", "Belarusian", "Беларуская мова"},
										 {"bg", 0, 0, "Bulgarian", "български"},
										 {"bg", "BG", 0, "Bulgarian (Bulgaria)", 0},
										 {"bn", 0, 0, "Bengali", "বাংলা"},
										 {"bn", "BD", 0, "Bengali (Bangladesh)", 0},
										 {"bn", "IN", 0, "Bengali (India)", 0},
										 {"bo", 0, 0, "Tibetan", "བོད་སྐད་"},
										 {"br", 0, 0, "Breton", "brezhoneg"},
										 {"bs", 0, 0, "Bosnian", "Bosanski"},
										 {"bs", "BA", 0, "Bosnian (Bosnia/Herzegovina)", 0},
										 {"bs", "BS", 0, "Bosnian (Bahamas)", 0},
										 {"ca", "ES", "valencia", "Catalan (valencia)", 0},
										 {"ca", "ES", 0, "Catalan (Spain)", 0},
										 {"ca", 0, "valencia", "Catalan (valencia)", 0},
										 {"ca", 0, 0, "Catalan", 0},
										 {"cmn", 0, 0, "Mandarin", 0},
										 {"co", 0, 0, "Corsican", "corsu"},
										 {"cs", 0, 0, "Czech", "Čeština"},
										 {"cs", "CZ", 0, "Czech (Czech Republic)", "Čeština (Česká Republika)"},
										 {"cy", 0, 0, "Welsh", "Welsh"},
										 {"cy", "GB", 0, "Welsh (Great Britain)", "Welsh (Great Britain)"},
										 {"cz", 0, 0, "Unknown language", "Unknown language"},
										 {"da", 0, 0, "Danish", "Dansk"},
										 {"da", "DK", 0, "Danish (Denmark)", "Dansk (Danmark)"},
										 {"de", 0, 0, "German", "Deutsch"},
										 {"de", "AT", 0, "German (Austria)", "Deutsch (Österreich)"},
										 {"de", "CH", 0, "German (Switzerland)", "Deutsch (Schweiz)"},
										 {"de", "DE", 0, "German (Germany)", "Deutsch (Deutschland)"},
										 {"dk", 0, 0, "Unknown language", "Unknown language"},
										 {"dz", 0, 0, "Dzongkha", "རྫོང་ཁ"},
										 {"el", 0, 0, "Greek", "ελληνικά"},
										 {"el", "GR", 0, "Greek (Greece)", 0},
										 {"en", 0, 0, "English", "English"},
										 {"en", "AU", 0, "English (Australia)", "English (Australia)"},
										 {"en", "CA", 0, "English (Canada)", "English (Canada)"},
										 {"en", "GB", 0, "English (Great Britain)", "English (Great Britain)"},
										 {"en", "US", 0, "English (United States)", "English (United States)"},
										 {"en", "ZA", 0, "English (South Africa)", "English (South Africa)"},
										 {"en", 0, "boldquot", "English", "English"},
										 {"en", 0, "quot", "English", "English"},
										 {"en", "US", "piglatin", "English", "English"},
										 {"eo", 0, 0, "Esperanto", "Esperanto"},
										 {"es", 0, 0, "Spanish", "Español"},
										 {"es", "AR", 0, "Spanish (Argentina)", 0},
										 {"es", "CL", 0, "Spanish (Chile)", 0},
										 {"es", "CO", 0, "Spanish (Colombia)", 0},
										 {"es", "CR", 0, "Spanish (Costa Rica)", 0},
										 {"es", "DO", 0, "Spanish (Dominican Republic)", 0},
										 {"es", "EC", 0, "Spanish (Ecuador)", 0},
										 {"es", "ES", 0, "Spanish (Spain)", 0},
										 {"es", "GT", 0, "Spanish (Guatemala)", 0},
										 {"es", "HN", 0, "Spanish (Honduras)", 0},
										 {"es", "LA", 0, "Spanish (Laos)", 0},
										 {"es", "MX", 0, "Spanish (Mexico)", 0},
										 {"es", "NI", 0, "Spanish (Nicaragua)", 0},
										 {"es", "PA", 0, "Spanish (Panama)", 0},
										 {"es", "PE", 0, "Spanish (Peru)", 0},
										 {"es", "PR", 0, "Spanish (Puerto Rico)", 0},
										 {"es", "SV", 0, "Spanish (El Salvador)", 0},
										 {"es", "UY", 0, "Spanish (Uruguay)", 0},
										 {"es", "VE", 0, "Spanish (Venezuela)", 0},
										 {"et", 0, 0, "Estonian", "eesti keel"},
										 {"et", "EE", 0, "Estonian (Estonia)", 0},
										 {"et", "ET", 0, "Estonian (Ethiopia)", 0},
										 {"eu", 0, 0, "Basque", "euskara"},
										 {"eu", "ES", 0, "Basque (Spain)", 0},
										 {"fa", 0, 0, "Persian", "فارسى"},
										 {"fa", "AF", 0, "Persian (Afghanistan)", 0},
										 {"fa", "IR", 0, "Persian (Iran)", 0},
										 {"fi", 0, 0, "Finnish", "suomi"},
										 {"fi", "FI", 0, "Finnish (Finland)", 0},
										 {"fo", 0, 0, "Faroese", "Føroyskt"},
										 {"fo", "FO", 0, "Faeroese (Faroe Islands)", 0},
										 {"fr", 0, 0, "French", "Français"},
										 {"fr", "CA", 0, "French (Canada)", "Français (Canada)"},
										 {"fr", "CH", 0, "French (Switzerland)", "Français (Suisse)"},
										 {"fr", "FR", 0, "French (France)", "Français (France)"},
										 {"fr", "LU", 0, "French (Luxembourg)", "Français (Luxembourg)"},
										 {"fy", 0, 0, "Frisian", "Frysk"},
										 {"ga", 0, 0, "Irish", "Gaeilge"},
										 {"gd", 0, 0, "Gaelic Scots", "Gàidhlig"},
										 {"gl", 0, 0, "Galician", "Galego"},
										 {"gl", "ES", 0, "Galician (Spain)", 0},
										 {"gn", 0, 0, "Guarani", "Avañe'ẽ"},
										 {"gu", 0, 0, "Gujarati", "ગુજરાતી"},
										 {"gv", 0, 0, "Manx", "Gaelg"},
										 {"ha", 0, 0, "Hausa", "حَوْسَ"},
										 {"he", 0, 0, "Hebrew", "עברית"},
										 {"he", "IL", 0, "Hebrew (Israel)", 0},
										 {"hi", 0, 0, "Hindi", "हिन्दी"},
										 {"hi", "IN", 0, "Hindi (India)", 0},
										 {"hr", 0, 0, "Croatian", "Hrvatski"},
										 {"hr", "HR", 0, "Croatian (Croatia)", 0},
										 {"hu", 0, 0, "Hungarian", "magyar"},
										 {"hu", "HU", 0, "Hungarian (Hungary)", 0},
										 {"hy", 0, 0, "Armenian", "Հայերեն"},
										 {"ia", 0, 0, "Interlingua", "Interlingua"},
										 {"id", 0, 0, "Indonesian", "Bahasa Indonesia"},
										 {"id", "ID", 0, "Indonesian (Indonesia)", 0},
										 {"is", 0, 0, "Icelandic", "Íslenska"},
										 {"is", "IS", 0, "Icelandic (Iceland)", 0},
										 {"it", 0, 0, "Italian", "Italiano"},
										 {"it", "CH", 0, "Italian (Switzerland)", 0},
										 {"it", "IT", 0, "Italian (Italy)", 0},
										 {"iu", 0, 0, "Inuktitut", "ᐃᓄᒃᑎᑐᑦ/inuktitut"},
										 {"ja", 0, 0, "Japanese", "日本語"},
										 {"ja", "JP", 0, "Japanese (Japan)", 0},
										 {"ka", 0, 0, "Georgian", "ქართული"},
										 {"kk", 0, 0, "Kazakh", "Қазақша"},
										 {"kl", 0, 0, "Kalaallisut", "Kalaallisut"},
										 {"km", 0, 0, "Khmer", "ភាសាខ្មែរ"},
										 {"km", "KH", 0, "Khmer (Cambodia)", 0},
										 {"kn", 0, 0, "Kannada", "ಕನ್ನಡ"},
										 {"ko", 0, 0, "Korean", "한국어"},
										 {"ko", "KR", 0, "Korean (Korea)", 0},
										 {"ku", 0, 0, "Kurdish", "Kurdî"},
										 {"kw", 0, 0, "Cornish", "Kernowek"},
										 {"ky", 0, 0, "Kirghiz", 0},
										 {"la", 0, 0, "Latin", "Latina"},
										 {"lo", 0, 0, "Lao", "ລາວ"},
										 {"lt", 0, 0, "Lithuanian", "Lietuvių"},
										 {"lt", "LT", 0, "Lithuanian (Lithuania)", 0},
										 {"lv", 0, 0, "Latvian", "Latviešu"},
										 {"lv", "LV", 0, "Latvian (Latvia)", 0},
										 {"jbo", 0, 0, "Lojban", "La .lojban."},
										 {"mg", 0, 0, "Malagasy", "Malagasy"},
										 {"mi", 0, 0, "Maori", "Māori"},
										 {"mk", 0, 0, "Macedonian", "Македонски"},
										 {"mk", "MK", 0, "Macedonian (Macedonia)", 0},
										 {"ml", 0, 0, "Malayalam", "മലയാളം"},
										 {"mn", 0, 0, "Mongolian", "Монгол"},
										 {"mr", 0, 0, "Marathi", "मराठी"},
										 {"ms", 0, 0, "Malay", "Bahasa Melayu"},
										 {"ms", "MY", 0, "Malay (Malaysia)", 0},
										 {"mt", 0, 0, "Maltese", "Malti"},
										 {"my", 0, 0, "Burmese", "မြန်မာဘာသာ"},
										 {"my", "MM", 0, "Burmese (Myanmar)", 0},
										 {"nb", 0, 0, "Norwegian Bokmal", 0},
										 {"nb", "NO", 0, "Norwegian Bokmål (Norway)", 0},
										 {"nds", 0, 0, "Low German", 0},
										 {"ne", 0, 0, "Nepali", 0},
										 {"nl", 0, 0, "Dutch", "Nederlands"},
										 {"nl", "BE", 0, "Dutch (Belgium)", 0},
										 {"nl", "NL", 0, "Dutch (Netherlands)", 0},
										 {"nn", 0, 0, "Norwegian Nynorsk", "Norsk nynorsk"},
										 {"nn", "NO", 0, "Norwegian Nynorsk (Norway)", 0},
										 {"no", 0, 0, "Norwegian", "Norsk bokmål"},
										 {"no", "NO", 0, "Norwegian (Norway)", 0},
										 {"no", "NY", 0, "Norwegian (NY)", 0},
										 {"nr", 0, 0, "Ndebele, South", 0},
										 {"oc", 0, 0, "Occitan post 1500", "Occitan"},
										 {"om", 0, 0, "Oromo", "Oromoo"},
										 {"or", 0, 0, "Oriya", "ଓଡ଼ିଆ"},
										 {"pa", 0, 0, "Punjabi", "ਪੰਜਾਬੀ"},
										 {"pl", 0, 0, "Polish", "Polski"},
										 {"pl", "PL", 0, "Polish (Poland)", 0},
										 {"ps", 0, 0, "Pashto", "پښتو"},
										 {"pt", 0, 0, "Portuguese", "Português"},
										 {"pt", "BR", 0, "Portuguese (Brazil)", 0},
										 {"pt", "PT", 0, "Portuguese (Portugal)", 0},
										 {"qu", 0, 0, "Quechua", "Runa Simi"},
										 {"rm", 0, 0, "Rhaeto-Romance", "Rumantsch"},
										 {"ro", 0, 0, "Romanian", "Română"},
										 {"ro", "RO", 0, "Romanian (Romania)", 0},
										 {"ru", 0, 0, "Russian", "Русский"},
										 {"ru", "RU", 0, "Russian (Russia", 0},
										 {"rw", 0, 0, "Kinyarwanda", "Kinyarwanda"},
										 {"sa", 0, 0, "Sanskrit", 0},
										 {"sd", 0, 0, "Sindhi", 0},
										 {"se", 0, 0, "Sami", "Sámegiella"},
										 {"se", "NO", 0, "Sami (Norway)", 0},
										 {"si", 0, 0, "Sinhalese", 0},
										 {"sk", 0, 0, "Slovak", "Slovenčina"},
										 {"sk", "SK", 0, "Slovak (Slovakia)", 0},
										 {"sl", 0, 0, "Slovenian", "Slovenščina"},
										 {"sl", "SI", 0, "Slovenian (Slovenia)", 0},
										 {"sl", "SL", 0, "Slovenian (Sierra Leone)", 0},
										 {"sm", 0, 0, "Samoan", 0},
										 {"so", 0, 0, "Somali", 0},
										 {"sp", 0, 0, "Unknown language", 0},
										 {"sq", 0, 0, "Albanian", "Shqip"},
										 {"sq", "AL", 0, "Albanian (Albania)", 0},
										 {"sr", 0, 0, "Serbian", "Српски / srpski"},
										 {"sr", "YU", 0, "Serbian (Yugoslavia)", 0},
										 {"sr", 0, "ije", "Serbian", 0},
										 {"sr", 0, "latin", "Serbian", 0},
										 {"sr", 0, "Latn", "Serbian", 0},
										 {"ss", 0, 0, "Swati", 0},
										 {"st", 0, 0, "Sotho", 0},
										 {"sv", 0, 0, "Swedish", "Svenska"},
										 {"sv", "SE", 0, "Swedish (Sweden)", 0},
										 {"sv", "SV", 0, "Swedish (El Salvador)", 0},
										 {"sw", 0, 0, "Swahili", 0},
										 {"ta", 0, 0, "Tamil", 0},
										 {"te", 0, 0, "Telugu", 0},
										 {"tg", 0, 0, "Tajik", 0},
										 {"th", 0, 0, "Thai", "ไทย"},
										 {"th", "TH", 0, "Thai (Thailand)", 0},
										 {"ti", 0, 0, "Tigrinya", 0},
										 {"tk", 0, 0, "Turkmen", 0},
										 {"tl", 0, 0, "Tagalog", 0},
										 {"to", 0, 0, "Tonga", 0},
										 {"tr", 0, 0, "Turkish", "Türkçe"},
										 {"tr", "TR", 0, "Turkish (Turkey)", 0},
										 {"ts", 0, 0, "Tsonga", 0},
										 {"tt", 0, 0, "Tatar", 0},
										 {"ug", 0, 0, "Uighur", 0},
										 {"uk", 0, 0, "Ukrainian", "Українська"},
										 {"uk", "UA", 0, "Ukrainian (Ukraine)", 0},
										 {"ur", 0, 0, "Urdu", 0},
										 {"ur", "PK", 0, "Urdu (Pakistan)", 0},
										 {"uz", 0, 0, "Uzbek", 0},
										 {"uz", 0, "cyrillic", "Uzbek", 0},
										 {"vi", 0, 0, "Vietnamese", "Tiếng Việt"},
										 {"vi", "VN", 0, "Vietnamese (Vietnam)", 0},
										 {"wa", 0, 0, "Walloon", 0},
										 {"wo", 0, 0, "Wolof", 0},
										 {"xh", 0, 0, "Xhosa", 0},
										 {"yi", 0, 0, "Yiddish", "ייִדיש"},
										 {"yo", 0, 0, "Yoruba", 0},
										 {"zh", 0, 0, "Chinese", "中文"},
										 {"zh", "CN", 0, "Chinese (simplified)", 0},
										 {"zh", "HK", 0, "Chinese (Hong Kong)", 0},
										 {"zh", "TW", 0, "Chinese (traditional)", 0},
										 {"zu", 0, 0, "Zulu", 0},
										 {nullptr, nullptr, nullptr, nullptr, nullptr}};
//*}

namespace {

core::String resolve_language_alias(const core::String &name) {
	typedef core::DynamicStringMap<core::String> Aliases;
	static Aliases language_aliases;
	if (language_aliases.empty()) {
		// FIXME: Many of those are not useful for us, since we leave
		// encoding to the app, not to the language, we could/should
		// also match against all language names, not just aliases from
		// locale.alias

		// Aliases taken from /etc/locale.alias
		language_aliases.put("bokmal", "nb_NO.ISO-8859-1");
		language_aliases.put("bokmål", "nb_NO.ISO-8859-1");
		language_aliases.put("catalan", "ca_ES.ISO-8859-1");
		language_aliases.put("croatian", "hr_HR.ISO-8859-2");
		language_aliases.put("czech", "cs_CZ.ISO-8859-2");
		language_aliases.put("danish", "da_DK.ISO-8859-1");
		language_aliases.put("dansk", "da_DK.ISO-8859-1");
		language_aliases.put("deutsch", "de_DE.ISO-8859-1");
		language_aliases.put("dutch", "nl_NL.ISO-8859-1");
		language_aliases.put("eesti", "et_EE.ISO-8859-1");
		language_aliases.put("estonian", "et_EE.ISO-8859-1");
		language_aliases.put("finnish", "fi_FI.ISO-8859-1");
		language_aliases.put("français", "fr_FR.ISO-8859-1");
		language_aliases.put("french", "fr_FR.ISO-8859-1");
		language_aliases.put("galego", "gl_ES.ISO-8859-1");
		language_aliases.put("galician", "gl_ES.ISO-8859-1");
		language_aliases.put("german", "de_DE.ISO-8859-1");
		language_aliases.put("greek", "el_GR.ISO-8859-7");
		language_aliases.put("hebrew", "he_IL.ISO-8859-8");
		language_aliases.put("hrvatski", "hr_HR.ISO-8859-2");
		language_aliases.put("hungarian", "hu_HU.ISO-8859-2");
		language_aliases.put("icelandic", "is_IS.ISO-8859-1");
		language_aliases.put("italian", "it_IT.ISO-8859-1");
		language_aliases.put("japanese", "ja_JP.eucJP");
		language_aliases.put("japanese.euc", "ja_JP.eucJP");
		language_aliases.put("ja_JP", "ja_JP.eucJP");
		language_aliases.put("ja_JP.ujis", "ja_JP.eucJP");
		language_aliases.put("japanese.sjis", "ja_JP.SJIS");
		language_aliases.put("korean", "ko_KR.eucKR");
		language_aliases.put("korean.euc", "ko_KR.eucKR");
		language_aliases.put("ko_KR", "ko_KR.eucKR");
		language_aliases.put("lithuanian", "lt_LT.ISO-8859-13");
		language_aliases.put("no_NO", "nb_NO.ISO-8859-1");
		language_aliases.put("no_NO.ISO-8859-1", "nb_NO.ISO-8859-1");
		language_aliases.put("norwegian", "nb_NO.ISO-8859-1");
		language_aliases.put("nynorsk", "nn_NO.ISO-8859-1");
		language_aliases.put("polish", "pl_PL.ISO-8859-2");
		language_aliases.put("portuguese", "pt_PT.ISO-8859-1");
		language_aliases.put("romanian", "ro_RO.ISO-8859-2");
		language_aliases.put("russian", "ru_RU.ISO-8859-5");
		language_aliases.put("slovak", "sk_SK.ISO-8859-2");
		language_aliases.put("slovene", "sl_SI.ISO-8859-2");
		language_aliases.put("slovenian", "sl_SI.ISO-8859-2");
		language_aliases.put("spanish", "es_ES.ISO-8859-1");
		language_aliases.put("swedish", "sv_SE.ISO-8859-1");
		language_aliases.put("thai", "th_TH.TIS-620");
		language_aliases.put("turkish", "tr_TR.ISO-8859-9");
	}

	Aliases::iterator i = language_aliases.find(name.toLower());
	if (i != language_aliases.end()) {
		return i->second;
	}
	return name;
}

} // namespace

Language Language::fromSpec(const core::String &language, const core::String &country, const core::String &modifier) {
	typedef core::DynamicStringMap<core::Buffer<const LanguageSpec *>> LanguageSpecMap;
	static LanguageSpecMap language_map;

	if (language_map.empty()) { // Init language_map
		for (int i = 0; languages[i].language != nullptr; ++i) {
			const core::String lng = languages[i].language;
			auto iter = language_map.find(lng);
			if (iter == language_map.end()) {
				language_map.put(lng, {&languages[i]});
			} else {
				iter->value.push_back(&languages[i]);
			}
		}
	}

	LanguageSpecMap::iterator i = language_map.find(language);
	if (i != language_map.end()) {
		core::Buffer<const LanguageSpec *> &lst = i->value;

		LanguageSpec tmpspec;
		tmpspec.language = language.c_str();
		tmpspec.country = country.empty() ? nullptr : country.c_str();
		tmpspec.modifier = modifier.empty() ? nullptr : modifier.c_str();
		Language tmplang(&tmpspec);

		const LanguageSpec *best_match = nullptr;
		int best_match_score = 0;
		for (const LanguageSpec* spec : lst) { // Search for the language that best matches the given spec, value country more then modifier
			int score = Language::match(Language(spec), tmplang);

			if (score > best_match_score) {
				best_match = spec;
				best_match_score = score;
			}
		}
		core_assert(best_match);
		return Language(best_match);
	}
	return Language();
}

Language Language::fromName(const core::String &spec_str) {
	return fromEnv(resolve_language_alias(spec_str));
}

Language Language::fromEnv(const core::String &env) {
	// Split LANGUAGE_COUNTRY.CODESET@MODIFIER into parts
	size_t ln = env.find("_");
	size_t dt = env.find(".");
	size_t at = env.find("@");

	core::String language;
	core::String country;
	core::String codeset;
	core::String modifier;

	// std::cout << ln << " " << dt << " " << at << std::endl;

	language = env.substr(0, core_min(core_min(ln, dt), at));

	if (ln != core::String::npos && ln + 1 < env.size()) // _
	{
		country = env.substr(ln + 1, (core_min(dt, at) == core::String::npos) ? core::String::npos
																			  : core_min(dt, at) - (ln + 1));
	}

	if (dt != core::String::npos && dt + 1 < env.size()) // .
	{
		codeset = env.substr(dt + 1, (at == core::String::npos) ? core::String::npos : (at - (dt + 1)));
	}

	if (at != core::String::npos && at + 1 < env.size()) // @
	{
		modifier = env.substr(at + 1);
	}

	return fromSpec(language, country, modifier);
}

Language::Language(const LanguageSpec *languageSpec) : _languageSpec(languageSpec) {
}

Language::Language() {
}

int Language::match(const Language &lhs, const Language &rhs) {
	if (lhs.getLanguage() != rhs.getLanguage()) {
		return 0;
	}
	static int match_tbl[3][3] = {
		// modifier match, wildchard, miss
		{9, 8, 5}, // country match
		{7, 6, 3}, // country wildcard
		{4, 2, 1}, // country miss
	};

	int c;
	if (lhs.getCountry() == rhs.getCountry())
		c = 0;
	else if (lhs.getCountry().empty() || rhs.getCountry().empty())
		c = 1;
	else
		c = 2;

	int m;
	if (lhs.getModifier() == rhs.getModifier())
		m = 0;
	else if (lhs.getModifier().empty() || rhs.getModifier().empty())
		m = 1;
	else
		m = 2;

	return match_tbl[c][m];
}

core::String Language::getLanguage() const {
	if (_languageSpec)
		return _languageSpec->language;
	return core::String::Empty;
}

core::String Language::getCountry() const {
	if (_languageSpec && _languageSpec->country)
		return _languageSpec->country;
	return core::String::Empty;
}

core::String Language::getModifier() const {
	if (_languageSpec && _languageSpec->modifier)
		return _languageSpec->modifier;
	return core::String::Empty;
}

core::String Language::getName() const {
	if (_languageSpec)
		return _languageSpec->name;
	return core::String::Empty;
}

core::String Language::getLocalizedName() const {
	if (_languageSpec && _languageSpec->name_localized)
		return _languageSpec->name_localized;
	return this->getName();
}

core::String Language::str() const {
	if (_languageSpec) {
		core::String var;
		var += _languageSpec->language;
		if (_languageSpec->country) {
			var += "_";
			var += _languageSpec->country;
		}

		if (_languageSpec->modifier) {
			var += "@";
			var += _languageSpec->modifier;
		}
		return var;
	}
	return core::String::Empty;
}

bool Language::operator==(const Language &rhs) const {
	return _languageSpec == rhs._languageSpec;
}

bool Language::operator!=(const Language &rhs) const {
	return _languageSpec != rhs._languageSpec;
}

} // namespace app
