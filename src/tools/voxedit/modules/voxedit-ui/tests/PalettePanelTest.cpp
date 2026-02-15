/**
 * @file
 */

#include "../PalettePanel.h"
#include "voxedit-ui/WindowTitles.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

/**
 * @brief Compute the screen-space center of a palette color cell.
 *
 * The palette grid starts at the window's content region top-left corner. Each cell is
 * a square of @c frameHeight pixels. Cells are laid out left-to-right and wrap when they
 * would exceed @c contentRegionWidth. The @c palettePanelIdx is the visual index (after
 * any sort reordering).
 */
static ImVec2 paletteCellCenter(ImGuiWindow *window, int palettePanelIdx) {
	const float frameHeight = ImGui::GetFrameHeight();
	const float startX = window->ContentRegionRect.Min.x;
	const float startY = window->ContentRegionRect.Min.y;
	const float availWidth = window->ContentRegionRect.Max.x - startX;
	const int cols = (int)(availWidth / frameHeight);
	const int col = palettePanelIdx % cols;
	const int row = palettePanelIdx / cols;
	return ImVec2(startX + col * frameHeight + frameHeight * 0.5f,
				  startY + row * frameHeight + frameHeight * 0.5f);
}

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
		ctx->Yield();

		ImGuiWindow *window = ImGui::FindWindowByName(id);
		IM_CHECK(window != nullptr);

		const palette::Palette& activePalette = _sceneMgr->activePalette();
		const color::RGBA slot0 = activePalette.color(0);
		const color::RGBA slot1 = activePalette.color(1);

		// compute screen positions for palette cells 0 and 1
		const ImVec2 pos0 = paletteCellCenter(window, 0);
		const ImVec2 pos1 = paletteCellCenter(window, 1);

		// simulate drag from cell 0 to cell 1 via mouse
		ctx->MouseMoveToPos(pos0);
		ctx->Yield();
		ctx->MouseDown(0);
		ctx->Yield();
		ctx->MouseMoveToPos(pos1);
		ctx->Yield();
		ctx->MouseUp(0);
		ctx->Yield();

		IM_CHECK_EQ(activePalette.color(0), slot1);
		IM_CHECK_EQ(activePalette.color(1), slot0);
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag and drop color ctrl")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		ctx->Yield();

		ImGuiWindow *window = ImGui::FindWindowByName(id);
		IM_CHECK(window != nullptr);

		const palette::Palette& activePalette = _sceneMgr->activePalette();
		const color::RGBA slot0 = activePalette.color(0);
		const color::RGBA slot1 = activePalette.color(1);
		const int index0 = activePalette.view().uiIndex(0);
		const int index1 = activePalette.view().uiIndex(1);

		// compute screen positions for palette cells 0 and 1
		const ImVec2 pos0 = paletteCellCenter(window, 0);
		const ImVec2 pos1 = paletteCellCenter(window, 1);

		// simulate drag from cell 0 to cell 1 with Ctrl held (reorder only)
		ctx->KeyDown(ImGuiMod_Ctrl);
		ctx->MouseMoveToPos(pos0);
		ctx->Yield();
		ctx->MouseDown(0);
		ctx->Yield();
		ctx->MouseMoveToPos(pos1);
		ctx->Yield();
		ctx->MouseUp(0);
		ctx->KeyUp(ImGuiMod_Ctrl);
		ctx->Yield();

		IM_CHECK_EQ(activePalette.color(0), slot0);
		IM_CHECK_EQ(activePalette.color(1), slot1);
		IM_CHECK_EQ(activePalette.view().uiIndex(0), index1);
		IM_CHECK_EQ(activePalette.view().uiIndex(1), index0);
		ctx->MenuClick("Sort/Original");
	};

	// multi select with shift+click
	IM_REGISTER_TEST(engine, testCategory(), "multi select shift click")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		ctx->Yield();

		ImGuiWindow *window = ImGui::FindWindowByName(id);
		IM_CHECK(window != nullptr);

		// click first cell
		const ImVec2 pos0 = paletteCellCenter(window, 0);
		ctx->MouseMoveToPos(pos0);
		ctx->MouseClick(0);
		ctx->Yield();

		// shift+click third cell to select range [0..2]
		const ImVec2 pos2 = paletteCellCenter(window, 2);
		ctx->KeyDown(ImGuiMod_Shift);
		ctx->MouseMoveToPos(pos2);
		ctx->MouseClick(0);
		ctx->KeyUp(ImGuiMod_Shift);
		ctx->Yield();
	};

	// set color name via context menu
	IM_REGISTER_TEST(engine, testCategory(), "set color name")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);
		ctx->Yield();

		ImGuiWindow *window = ImGui::FindWindowByName(id);
		IM_CHECK(window != nullptr);

		// right-click first cell to open context menu
		const ImVec2 pos0 = paletteCellCenter(window, 0);
		ctx->MouseMoveToPos(pos0);
		ctx->MouseClick(ImGuiMouseButton_Right);
		ctx->Yield();

		// type a color name into the Name input field
		ctx->ItemInputValue("//$FOCUSED/Name", "TestColor");
		ctx->Yield();

		// close the context menu
		ctx->KeyPress(ImGuiKey_Escape);
	};

	// test palette tools menu features
	IM_REGISTER_TEST(engine, testCategory(), "tools menu")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);

		// sort options
		ctx->MenuClick("Sort/Hue");
		ctx->Yield();
		ctx->MenuClick("Sort/Saturation");
		ctx->Yield();
		ctx->MenuClick("Sort/Brightness");
		ctx->Yield();
		ctx->MenuClick("Sort/CIELab");
		ctx->Yield();
		ctx->MenuClick("Sort/Original");
		ctx->Yield();

		// tools menu
		ctx->MenuClick("Tools/Remove unused color");
		ctx->Yield();
		ctx->MenuClick("Tools/Contrast stretching");
		ctx->Yield();
		ctx->MenuClick("Tools/White balancing");
		ctx->Yield();
	};

	// test palette modify sub-menu
	IM_REGISTER_TEST(engine, testCategory(), "tools modify")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, id));
		ctx->SetRef(id);

		ctx->MenuClick("Tools/Modify/Warmer");
		ctx->Yield();
		ctx->MenuClick("Tools/Modify/Colder");
		ctx->Yield();
		ctx->MenuClick("Tools/Modify/Brighter");
		ctx->Yield();
		ctx->MenuClick("Tools/Modify/Darker");
		ctx->Yield();
	};
}

} // namespace voxedit
