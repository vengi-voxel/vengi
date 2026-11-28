/**
 * @file
 */

#include "ui/font/FontResolver.h"
#include "app/tests/AbstractTest.h"
#include "ui/dearimgui/imgui.h"
#include "ui/dearimgui/imgui_internal.h"
#include <gtest/gtest.h>

namespace ui {
namespace font {

class FontResolverTest : public app::AbstractTest {};

TEST_F(FontResolverTest, testSelectFallbackPreferNotoSansCJK) {
	const core::DynamicArray<core::String> input = {
		"C:\\Windows\\Fonts\\meiryo.ttc",
		"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
		"C:\\Windows\\Fonts\\YuGothR.ttc",
	};
	const core::DynamicArray<core::String> result = selectFallbackSystemFonts(input);
	ASSERT_EQ(3u, result.size());
	EXPECT_EQ("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc", result[0]);
	EXPECT_EQ("C:\\Windows\\Fonts\\meiryo.ttc", result[1]);
	EXPECT_EQ("C:\\Windows\\Fonts\\YuGothR.ttc", result[2]);
}

TEST_F(FontResolverTest, testSelectFallbackWindowsOrder) {
	const core::DynamicArray<core::String> input = {
		"C:\\Windows\\Fonts\\arial.ttf",
		"C:\\Windows\\Fonts\\meiryob.ttc",
		"C:\\Windows\\Fonts\\NotoSansJP-VF.ttf",
		"C:\\Windows\\Fonts\\msgothic.ttc",
		"C:\\Windows\\Fonts\\YuGothR.ttc",
		"C:\\Windows\\Fonts\\YuGothM.ttc",
		"C:\\Windows\\Fonts\\BIZ-UDGothicR.ttc",
		"C:\\Windows\\Fonts\\meiryo.ttc",
	};
	const core::DynamicArray<core::String> result = selectFallbackSystemFonts(input);
	ASSERT_EQ(6u, result.size());
	EXPECT_EQ("C:\\Windows\\Fonts\\meiryo.ttc", result[0]);
	EXPECT_EQ("C:\\Windows\\Fonts\\BIZ-UDGothicR.ttc", result[1]);
	EXPECT_EQ("C:\\Windows\\Fonts\\YuGothM.ttc", result[2]);
	EXPECT_EQ("C:\\Windows\\Fonts\\YuGothR.ttc", result[3]);
	EXPECT_EQ("C:\\Windows\\Fonts\\msgothic.ttc", result[4]);
	EXPECT_EQ("C:\\Windows\\Fonts\\NotoSansJP-VF.ttf", result[5]);
}

TEST_F(FontResolverTest, testSelectFallbackMatchesFilenameOnly) {
	const core::DynamicArray<core::String> input = {
		"C:\\meiryo.ttc\\NotoSansCJK\\arial.ttf",
		"/home/NotoSansCJK/meiryo.ttc/times.ttf",
		"C:\\Windows\\Fonts\\MEIRYO.TTC",
	};
	const core::DynamicArray<core::String> result = selectFallbackSystemFonts(input);
	ASSERT_EQ(1u, result.size());
	EXPECT_EQ("C:\\Windows\\Fonts\\MEIRYO.TTC", result[0]);
}

TEST_F(FontResolverTest, testSelectFallbackDeduplicatesAndEmpty) {
	const core::DynamicArray<core::String> emptyInput;
	EXPECT_TRUE(selectFallbackSystemFonts(emptyInput).empty());

	const core::DynamicArray<core::String> dupInput = {
		"C:\\Windows\\Fonts\\NotoSansJP-Regular.ttf",
		"C:\\Windows\\Fonts\\NotoSansJP-Regular.ttf",
	};
	const core::DynamicArray<core::String> result = selectFallbackSystemFonts(dupInput);
	ASSERT_EQ(1u, result.size());
	EXPECT_EQ("C:\\Windows\\Fonts\\NotoSansJP-Regular.ttf", result[0]);
}

TEST_F(FontResolverTest, testSelectFallbackByLanguage) {
	const core::DynamicArray<core::String> osFonts = {
		"C:\\Windows\\Fonts\\arial.ttf",
		"C:\\Windows\\Fonts\\meiryo.ttc",
		"C:\\Windows\\Fonts\\msyh.ttc",
		"C:\\Windows\\Fonts\\msjh.ttc",
		"C:\\Windows\\Fonts\\malgun.ttf",
		"/System/Library/Fonts/Hiragino Sans.ttc",
		"/System/Library/Fonts/PingFang.ttc",
		"/System/Library/Fonts/AppleSDGothicNeo.ttc",
	};

	const core::DynamicArray<core::String> jaFonts = selectFallbackSystemFonts(osFonts, "ja_JP");
	ASSERT_FALSE(jaFonts.empty());
	EXPECT_EQ("C:\\Windows\\Fonts\\meiryo.ttc", jaFonts[0]);

	const core::DynamicArray<core::String> scFonts = selectFallbackSystemFonts(osFonts, "zh_CN");
	ASSERT_FALSE(scFonts.empty());
	EXPECT_EQ("C:\\Windows\\Fonts\\msyh.ttc", scFonts[0]);

	const core::DynamicArray<core::String> tcFonts = selectFallbackSystemFonts(osFonts, "zh_TW");
	ASSERT_FALSE(tcFonts.empty());
	EXPECT_EQ("C:\\Windows\\Fonts\\msjh.ttc", tcFonts[0]);

	const core::DynamicArray<core::String> koFonts = selectFallbackSystemFonts(osFonts, "ko_KR");
	ASSERT_FALSE(koFonts.empty());
	EXPECT_EQ("C:\\Windows\\Fonts\\malgun.ttf", koFonts[0]);

	const core::DynamicArray<core::String> cjkFonts = {
		"/usr/share/fonts/opentype/noto/NotoSansCJKkr-Regular.otf",
		"/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
		"/usr/share/fonts/opentype/noto/NotoSansCJKtc-Regular.otf",
		"/usr/share/fonts/opentype/noto/NotoSansCJKjp-Regular.otf",
	};
	EXPECT_EQ("/usr/share/fonts/opentype/noto/NotoSansCJKjp-Regular.otf",
			  selectFallbackSystemFonts(cjkFonts, "ja")[0]);
	EXPECT_EQ("/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
			  selectFallbackSystemFonts(cjkFonts, "zh_CN")[0]);
	EXPECT_EQ("/usr/share/fonts/opentype/noto/NotoSansCJKtc-Regular.otf",
			  selectFallbackSystemFonts(cjkFonts, "zh_HK")[0]);
	EXPECT_EQ("/usr/share/fonts/opentype/noto/NotoSansCJKkr-Regular.otf",
			  selectFallbackSystemFonts(cjkFonts, "ko_KR")[0]);
}

#ifdef _WIN32
TEST_F(FontResolverTest, testLoadSystemJapaneseFontOnWindows) {
	const core::DynamicArray<core::String> &systemFonts = findSystemFonts();
	ASSERT_FALSE(systemFonts.empty());
	const core::DynamicArray<core::String> &fallbackFonts = selectFallbackSystemFonts(systemFonts);
	ASSERT_FALSE(fallbackFonts.empty());

	ImGuiContext *ctx = ImGui::CreateContext();
	ASSERT_NE(ctx, nullptr);
	ImGuiIO &io = ImGui::GetIO();
	ImFont *baseFont = io.Fonts->AddFontDefaultVector();
	ASSERT_NE(baseFont, nullptr);

	bool loaded = false;
	for (const core::String &fontPath : fallbackFonts) {
		ImFontConfig fontCfg;
		fontCfg.MergeMode = true;
		fontCfg.Flags |= ImFontFlags_NoLoadError;
		if (io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 0.0f, &fontCfg) != nullptr) {
			loaded = true;
			break;
		}
	}
	EXPECT_TRUE(loaded);
	io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
	ImFontAtlasBuildMain(io.Fonts);
	ASSERT_NE(io.Fonts->TexData, nullptr);
	EXPECT_GT(io.Fonts->TexData->Width, 0);
	EXPECT_GT(io.Fonts->TexData->Height, 0);
	EXPECT_TRUE(baseFont->IsGlyphInFont(0x3042)); // Hiragana A
	EXPECT_TRUE(baseFont->IsGlyphInFont(0x6F22)); // Kanji Kan
	ImFontBaked *baked = baseFont->GetFontBaked(14.0f);
	ASSERT_NE(baked, nullptr);
	EXPECT_NE(baked->FindGlyphNoFallback(0x3042), nullptr);
	EXPECT_NE(baked->FindGlyphNoFallback(0x6F22), nullptr);
	ImGui::DestroyContext(ctx);
}
#endif

} // namespace font
} // namespace ui
