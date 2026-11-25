/**
 * @file
 */

#include "ui/font/FontResolver.h"
#include "core/ArrayLength.h"
#include "core/StringUtil.h"

namespace ui {
namespace font {

static core::String extractBasename(const core::String &fontPath) {
	const size_t slashPos = fontPath.rfind('/');
	const size_t backslashPos = fontPath.rfind('\\');
	size_t sepPos = core::String::npos;
	if (slashPos != core::String::npos && backslashPos != core::String::npos) {
		sepPos = slashPos > backslashPos ? slashPos : backslashPos;
	} else if (slashPos != core::String::npos) {
		sepPos = slashPos;
	} else {
		sepPos = backslashPos;
	}
	if (sepPos == core::String::npos) {
		return fontPath;
	}
	return fontPath.substr(sepPos + 1);
}

static bool containsPath(const core::DynamicArray<core::String> &paths, const core::String &target) {
	for (const core::String &path : paths) {
		if (path == target) {
			return true;
		}
	}
	return false;
}

static void appendMatchingFonts(const core::DynamicArray<core::String> &systemFonts, const char *const *patterns,
								int count, core::DynamicArray<core::String> &results) {
	for (int i = 0; i < count; ++i) {
		const char *pattern = patterns[i];
		for (const core::String &fontPath : systemFonts) {
			const core::String filename = extractBasename(fontPath);
			if (!core::string::icontains(filename, pattern)) {
				continue;
			}
			if (!containsPath(results, fontPath)) {
				results.push_back(fontPath);
			}
		}
	}
}

static bool isLanguagePrefix(const core::String &lang, const char *code) {
	if (lang == code) {
		return true;
	}
	const core::String underscorePrefix = core::String::format("%s_", code);
	const core::String dashPrefix = core::String::format("%s-", code);
	return core::string::startsWith(lang, underscorePrefix.c_str()) ||
		   core::string::startsWith(lang, dashPrefix.c_str());
}

core::DynamicArray<core::String> selectFallbackSystemFonts(const core::DynamicArray<core::String> &systemFonts,
														   const core::String &language) {
	static const char *jaPatterns[] = {
		"NotoSansCJKjp",
		"NotoSansCJK",
		"NotoSansJP-Regular",
		"meiryo.ttc",
		"BIZ-UDGothicR.ttc",
		"YuGothM.ttc",
		"YuGothR.ttc",
		"msgothic.ttc",
		"NotoSansJP",
		"Hiragino Sans",
		"Hiragino Kaku Gothic",
	};
	static const char *jaFallback[] = {
		"msyh.ttc",
		"msjh.ttc",
		"malgun.ttf",
		"PingFang",
		"AppleSDGothicNeo",
	};

	static const char *tcPatterns[] = {
		"NotoSansCJKtc",
		"NotoSansCJK",
		"NotoSansTC-Regular",
		"msjh.ttc",
		"mingliu.ttc",
		"PingFang",
		"NotoSansTC",
	};
	static const char *tcFallback[] = {
		"msyh.ttc",
		"meiryo.ttc",
		"YuGothM.ttc",
		"YuGothR.ttc",
		"malgun.ttf",
		"Hiragino Sans",
		"AppleSDGothicNeo",
	};

	static const char *scPatterns[] = {
		"NotoSansCJKsc",
		"NotoSansCJK",
		"NotoSansSC-Regular",
		"msyh.ttc",
		"simhei.ttf",
		"simsun.ttc",
		"PingFang",
		"NotoSansSC",
	};
	static const char *scFallback[] = {
		"msjh.ttc",
		"meiryo.ttc",
		"YuGothM.ttc",
		"YuGothR.ttc",
		"malgun.ttf",
		"Hiragino Sans",
		"AppleSDGothicNeo",
	};

	static const char *koPatterns[] = {
		"NotoSansCJKkr",
		"NotoSansCJK",
		"NotoSansKR-Regular",
		"malgun.ttf",
		"gulim.ttc",
		"AppleSDGothicNeo",
		"NotoSansKR",
	};
	static const char *koFallback[] = {
		"meiryo.ttc",
		"YuGothM.ttc",
		"YuGothR.ttc",
		"msyh.ttc",
		"msjh.ttc",
		"Hiragino Sans",
		"PingFang",
	};

	static const char *defaultPatterns[] = {
		"NotoSansCJK",
		"NotoSansJP-Regular",
		"meiryo.ttc",
		"BIZ-UDGothicR.ttc",
		"YuGothM.ttc",
		"YuGothR.ttc",
		"msgothic.ttc",
		"NotoSansJP",
		"msyh.ttc",
		"msjh.ttc",
		"malgun.ttf",
		"Hiragino Sans",
		"Hiragino Kaku Gothic",
		"PingFang",
		"AppleSDGothicNeo",
	};

	core::DynamicArray<core::String> results;
	const core::String lang = language.toLower();

	if (isLanguagePrefix(lang, "ja")) {
		appendMatchingFonts(systemFonts, jaPatterns, lengthof(jaPatterns), results);
		appendMatchingFonts(systemFonts, jaFallback, lengthof(jaFallback), results);
	} else if (isLanguagePrefix(lang, "zh")) {
		const bool isTraditional = core::string::icontains(lang, "tw") || core::string::icontains(lang, "hk") ||
								   core::string::icontains(lang, "mo") || core::string::icontains(lang, "hant");
		if (isTraditional) {
			appendMatchingFonts(systemFonts, tcPatterns, lengthof(tcPatterns), results);
			appendMatchingFonts(systemFonts, tcFallback, lengthof(tcFallback), results);
		} else {
			appendMatchingFonts(systemFonts, scPatterns, lengthof(scPatterns), results);
			appendMatchingFonts(systemFonts, scFallback, lengthof(scFallback), results);
		}
	} else if (isLanguagePrefix(lang, "ko")) {
		appendMatchingFonts(systemFonts, koPatterns, lengthof(koPatterns), results);
		appendMatchingFonts(systemFonts, koFallback, lengthof(koFallback), results);
	} else {
		appendMatchingFonts(systemFonts, defaultPatterns, lengthof(defaultPatterns), results);
	}

	return results;
}

} // namespace font
} // namespace ui
