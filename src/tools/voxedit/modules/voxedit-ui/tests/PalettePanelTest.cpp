/**
 * @file
 */

#include "../PalettePanel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

void PalettePanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "switch built-in")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		for (int i = lengthof(palette::Palette::builtIn) - 1; i >= 0; --i) {
			ctx->SetRef(id);
			ctx->MenuClick("File/Switch");
			ctx->SetRef(POPUP_TITLE_LOAD_PALETTE);
			ctx->ItemClick("##type");
			core::String name = core::String::format("//$FOCUSED/%s", palette::Palette::builtIn[i]);
			ctx->ItemClick(name.c_str());
			ctx->ItemClick("###Ok");
			palette::Palette check;
			check.load(palette::Palette::builtIn[i]);
			const palette::Palette& activePalette = _sceneMgr->activePalette();
			IM_CHECK_EQ(activePalette.colorCount(), check.colorCount());
			IM_CHECK_EQ(activePalette.color(0), check.color(0));
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "lospec")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		ctx->MenuClick("File/Lospec/ID");
		ctx->ItemInputValue("//$FOCUSED/ID", "commodore64");
		ctx->ItemClick("//$FOCUSED/Ok");
		ctx->MenuClick("File/Export");
		const palette::Palette& activePalette = _sceneMgr->activePalette();
		IM_CHECK_EQ(activePalette.colorCount(), 16);
		IM_CHECK_EQ(activePalette.color(0), color::RGBA(0, 0, 0, 255));
		IM_CHECK_EQ(activePalette.color(4), color::RGBA(255, 255, 255, 255));
		saveFile(ctx, "palette-lospec.png");
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag and drop color")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		const palette::Palette& activePalette = _sceneMgr->activePalette();
		const color::RGBA slot0 = activePalette.color(0);
		const color::RGBA slot1 = activePalette.color(1);
		// TODO: these items don't exist anymore due to performance optimizations. Only that one item which is hovered is submitted. Everything else is just a drawlist draw command
		// this means that this call doesn't work anymore
		ctx->ItemDragAndDrop("$$0", "$$1");
		ctx->Yield();
		IM_CHECK_EQ(activePalette.color(0), slot1);
		IM_CHECK_EQ(activePalette.color(1), slot0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag and drop color ctrl")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		const palette::Palette& activePalette = _sceneMgr->activePalette();
		const color::RGBA slot0 = activePalette.color(0);
		const color::RGBA slot1 = activePalette.color(1);
		const int index0 = activePalette.view().uiIndex(0);
		const int index1 = activePalette.view().uiIndex(1);
		ctx->KeyDown(ImGuiMod_Ctrl);
		// TODO: these items don't exist anymore due to performance optimizations. Only that one item which is hovered is submitted. Everything else is just a drawlist draw command
		// this means that this call doesn't work anymore
		ctx->ItemDragAndDrop("$$0", "$$1");
		ctx->KeyUp(ImGuiMod_Ctrl);
		IM_CHECK_EQ(activePalette.color(0), slot0);
		IM_CHECK_EQ(activePalette.color(1), slot1);
		IM_CHECK_EQ(activePalette.view().uiIndex(0), index1);
		IM_CHECK_EQ(activePalette.view().uiIndex(1), index0);
		ctx->MenuClick("Sort/Original");
	};
}

} // namespace voxedit
