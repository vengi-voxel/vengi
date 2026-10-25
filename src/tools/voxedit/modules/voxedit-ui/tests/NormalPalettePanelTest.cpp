/**
 * @file
 */

#include "../NormalPalettePanel.h"
#include "TestUtil.h"
#include "command/CommandHandler.h"
#include "memento/MementoHandler.h"
#include "palette/NormalPalette.h"
#include "util/VarUtil.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"

namespace voxedit {

void NormalPalettePanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "palette cell size options")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		util::ScopedVarChange size(cfg::VoxEditPaletteColorSize, "1.0");
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Tiberian Sun");
		for (float value : {0.5f, 1.0f, 2.0f, 4.0f}) {
			ctx->SetRef(id);
			ctx->MenuClick("Options");
			ctx->ItemInputValue("//$FOCUSED/Palette color size", value);
			IM_CHECK_EQ(core::getVar(cfg::VoxEditPaletteColorSize)->floatVal(), value);
			ctx->KeyPress(ImGuiKey_Escape);
			ctx->SetRef(id);
			ImGuiWindow *window = ImGui::FindWindowByName(id);
			IM_CHECK(window != nullptr);
			ctx->ScrollToTop(window->ID);
			ctx->Yield(2);
			const float cellSize = ImGui::GetFrameHeight() * value;
			const int columns = core_max(1, (int)(window->ContentRegionRect.GetWidth() / cellSize));
			for (int index : {0, columns - 1, columns}) {
				const core::String label = core::String::format("normal-%i", index);
				const ImGuiTestItemInfo item = ctx->ItemInfo(label.c_str());
				IM_CHECK_EQ(item.RectFull.GetWidth(), cellSize);
				IM_CHECK_EQ(item.RectFull.GetHeight(), cellSize);
				ctx->ItemClick(label.c_str());
				IM_CHECK_EQ(_selectedIndex, index);
				IM_CHECK_EQ(_sceneMgr->modifier().normalColorIndex(), index);
			}
		}
	};

	IM_REGISTER_TEST(engine, testCategory(), "create and edit normal entries")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		util::ScopedVarChange size(cfg::VoxEditPaletteColorSize, "1.0");
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));
		scenegraph::SceneGraphNode &node = *_sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
		palette::NormalPalette initial;
		initial.setNormal(0, glm::vec3(1, 0, 0));
		node.setNormalPalette(initial);
		_sceneMgr->modifier().setNormalColorIndex(0);
		_sceneMgr->mementoHandler().markNormalPaletteChange(_sceneMgr->sceneGraph(), node);
		ctx->Yield(2);
		ctx->ItemClick("normal-4");
		IM_CHECK_EQ(_selectedIndex, 4);
		IM_CHECK_EQ(_sceneMgr->modifier().normalColorIndex(), 0);

		ctx->ItemInputValue("Normal/$$1", 0.0f);
		IM_CHECK(!_targetValid);
		IM_CHECK((ctx->ItemInfo("Add normal").ItemFlags & ImGuiItemFlags_Disabled) != 0);
		IM_CHECK_EQ(initial.hash(), node.normalPalette().hash());
		ctx->ItemClick("+Z");
		const color::RGBA z = palette::NormalPalette::toRGBA(glm::vec3(0, 0, 1));
		IM_CHECK_EQ(node.normalPalette().normal(4), z);
		IM_CHECK_EQ(node.normalPalette().size(), 5u);
		IM_CHECK_EQ(_selectedIndex, 4);
		IM_CHECK_EQ(_sceneMgr->modifier().normalColorIndex(), 4);
		// Verify that the rendered swatch follows the edit, not only the stored vector.
		ctx->MouseMove("normal-4");
		ctx->Yield(2);
		const ImRect rect = ctx->ItemInfo("normal-4").RectFull;
		ImGuiWindow *window = ImGui::FindWindowByName(id);
		IM_CHECK(window != nullptr);
		bool swatchUpdated = false;
		for (const ImDrawVert &vertex : window->DrawList->VtxBuffer) {
			if (vertex.col == z.rgba && rect.Contains(vertex.pos)) {
				swatchUpdated = true;
				break;
			}
		}
		IM_CHECK(swatchUpdated);
		IM_CHECK(_sceneMgr->undo());
		ctx->Yield(2);
		IM_CHECK_EQ(node.normalPalette().size(), 1u);
		IM_CHECK_EQ(_sceneMgr->modifier().normalColorIndex(), 0);
		IM_CHECK(_sceneMgr->redo());
		ctx->Yield(2);
		ctx->ItemClick("normal-4");
		ctx->ItemClick("-X");
		IM_CHECK_EQ(node.normalPalette().normal(4), palette::NormalPalette::toRGBA(glm::vec3(-1, 0, 0)));
		IM_CHECK(_sceneMgr->undo());
		IM_CHECK_EQ(node.normalPalette().normal(4), z);
	};

	IM_REGISTER_TEST(engine, testCategory(), "live normal slider undo")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		util::ScopedVarChange size(cfg::VoxEditPaletteColorSize, "1.0");
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));
		scenegraph::SceneGraphNode &node = *_sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
		palette::NormalPalette initial;
		initial.setNormal(0, glm::vec3(1, 0, 0));
		node.setNormalPalette(initial);
		_sceneMgr->modifier().setNormalColorIndex(0);
		_sceneMgr->mementoHandler().markNormalPaletteChange(_sceneMgr->sceneGraph(), node);
		ctx->Yield(2);
		ctx->ItemClick("normal-0");
		ctx->MouseMove("Longitude");
		const ImVec2 grab = ctx->ItemInfo("Longitude").RectFull.GetCenter();
		const size_t states = _sceneMgr->mementoHandler().stateSize();
		ctx->MouseDown();
		ctx->MouseMoveToPos(ImVec2(grab.x + 25.0f, grab.y));
		ctx->Yield(2);
		IM_CHECK_NE(node.normalPalette().hash(), initial.hash());
		IM_CHECK_EQ(_sceneMgr->mementoHandler().stateSize(), states);
		ctx->MouseMoveToPos(ImVec2(grab.x + 40.0f, grab.y));
		ctx->Yield(2);
		IM_CHECK_EQ(_sceneMgr->mementoHandler().stateSize(), states);
		ctx->MouseUp();
		ctx->Yield(2);
		IM_CHECK_EQ(_sceneMgr->mementoHandler().stateSize(), states + 1);
		const uint32_t changed = node.normalPalette().hash();
		IM_CHECK(_sceneMgr->undo());
		IM_CHECK_EQ(node.normalPalette().hash(), initial.hash());
		IM_CHECK(_sceneMgr->redo());
		IM_CHECK_EQ(node.normalPalette().hash(), changed);
	};

	IM_REGISTER_TEST(engine, testCategory(), "copy paste and duplicate normals")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		util::ScopedVarChange size(cfg::VoxEditPaletteColorSize, "1.0");
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));
		_hasCopiedNormal = false;
		scenegraph::SceneGraphNode &source = *_sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
		palette::NormalPalette normals;
		normals.setNormal(0, glm::vec3(1, 0, 0));
		source.setNormalPalette(normals);
		_sceneMgr->modifier().setNormalColorIndex(0);
		ctx->Yield(2);
		ctx->ItemClick("normal-0", ImGuiMouseButton_Right);
		IM_CHECK((ctx->ItemInfo("//$FOCUSED/Paste normal").ItemFlags & ImGuiItemFlags_Disabled) != 0);
		ctx->ItemClick("//$FOCUSED/Copy normal");
		const color::RGBA copied = normals.normal(0);
		IM_CHECK(_hasCopiedNormal);
		IM_CHECK(_sceneMgr->nodeSetNormal(source.uuid(), 0, palette::NormalPalette::toRGBA(glm::vec3(0, 0, 1))));
		const int targetId = _sceneMgr->addModelChild("normal paste", 8, 8, 8);
		IM_CHECK(targetId != InvalidNodeId);
		scenegraph::SceneGraphNode &target = *_sceneMgr->sceneGraphNode(targetId);
		_sceneMgr->nodeActivate(target.uuid());
		palette::NormalPalette targetPalette;
		targetPalette.setNormal(0, glm::vec3(0, 1, 0));
		target.setNormalPalette(targetPalette);
		_sceneMgr->mementoHandler().markNormalPaletteChange(_sceneMgr->sceneGraph(), target);
		IM_CHECK(focusWindow(ctx, id));
		ctx->Yield(2);
		ctx->ItemClick("normal-4", ImGuiMouseButton_Right);
		ctx->ItemClick("//$FOCUSED/Paste normal");
		IM_CHECK_EQ(target.normalPalette().normal(4), copied);
		IM_CHECK_EQ(_selectedIndex, 4);
		IM_CHECK(_sceneMgr->undo());
		IM_CHECK_EQ(target.normalPalette().size(), 1u);
		IM_CHECK(_sceneMgr->redo());
		ctx->Yield(2);
		ctx->ItemClick("normal-4", ImGuiMouseButton_Right);
		ctx->ItemClick("//$FOCUSED/Duplicate normal");
		IM_CHECK_EQ(target.normalPalette().normal(1), copied);
		IM_CHECK_EQ(_selectedIndex, 1);
		ctx->MouseMove("normal-4");
		ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_C);
		IM_CHECK(_sceneMgr->nodeSetNormal(target.uuid(), 4, targetPalette.normal(0)));
		ctx->MouseMove("normal-5");
		ctx->KeyPress(ImGuiMod_Ctrl | ImGuiKey_V);
		IM_CHECK_EQ(target.normalPalette().normal(5), copied);
	};

	IM_REGISTER_TEST(engine, testCategory(), "drag swap normal entries")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		util::ScopedVarChange size(cfg::VoxEditPaletteColorSize, "1.0");
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));
		scenegraph::SceneGraphNode &node = *_sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
		palette::NormalPalette initial;
		initial.setNormal(0, glm::vec3(1, 0, 0));
		initial.setNormal(1, glm::vec3(0, 1, 0));
		node.setNormalPalette(initial);
		_sceneMgr->mementoHandler().markNormalPaletteChange(_sceneMgr->sceneGraph(), node);
		ctx->Yield(2);
		const ImVec2 first = ctx->ItemInfo("normal-0").RectFull.GetCenter();
		const ImVec2 second = ctx->ItemInfo("normal-1").RectFull.GetCenter();
		dragFromTo(ctx, first, second);
		IM_CHECK_EQ(node.normalPalette().normal(0), initial.normal(1));
		IM_CHECK_EQ(node.normalPalette().normal(1), initial.normal(0));
		IM_CHECK(_sceneMgr->undo());
		IM_CHECK_EQ(node.normalPalette().hash(), initial.hash());
	};

	IM_REGISTER_TEST(engine, testCategory(), "check existance")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		ImGuiWindow *window = ImGui::FindWindowByName(id);
		IM_CHECK(window != nullptr);
		IM_CHECK(window->Active);
	};

	IM_REGISTER_TEST(engine, testCategory(), "no existance")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		ImGuiWindow *window = ImGui::FindWindowByName(id);
		IM_CHECK(window != nullptr);
		IM_CHECK(!window->Active);
	};

	IM_REGISTER_TEST(engine, testCategory(), "switch built-in")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Tiberian Sun");
		ctx->MenuClick("File/Red Alert 2");
		ctx->MenuClick("File/Slab6");
	};

	// auto normals and remove all normals afterwards
	IM_REGISTER_TEST(engine, testCategory(), "auto normals")->TestFunc = [=](ImGuiTestContext *ctx) {
		// load a template model so there are voxels to calculate normals for
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(newTemplateScene(ctx, "##templates/##River"));
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));

		// remove all normals first
		ctx->MenuClick("File/Remove all normals");
		ctx->Yield();

		// calculate normals with default (Flat) mode
		ctx->MenuClick("File/Auto normals/Calculate normals");
		ctx->Yield();
	};

	// export normal palette
	IM_REGISTER_TEST(engine, testCategory(), "export normal palette")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));
		ctx->MenuClick("File/Export");
		ctx->Yield();
		IM_CHECK(saveFile(ctx, "normalpalette-export.png"));
	};

	// change longitude and latitude and verify values change
	IM_REGISTER_TEST(engine, testCategory(), "longitude latitude")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));

		const glm::vec3 beforeLon = _targetNormal;
		changeSlider(ctx, "Longitude", true);
		ctx->Yield();
		const glm::vec3 afterLon = _targetNormal;
		IM_CHECK(afterLon.x != beforeLon.x || afterLon.z != beforeLon.z);

		changeSlider(ctx, "Longitude", false);

		const glm::vec3 beforeLat = _targetNormal;
		changeSlider(ctx, "Latitude", true);
		ctx->Yield();
		const glm::vec3 afterLat = _targetNormal;
		IM_CHECK(afterLat.y != beforeLat.y || afterLat.x != beforeLat.x || afterLat.z != beforeLat.z);

		changeSlider(ctx, "Latitude", false);
	};

	IM_REGISTER_TEST(engine, testCategory(), "flip buttons")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));

		const glm::vec3 before = _targetNormal;
		ctx->ItemClick("##flipnormal/X");
		ctx->Yield();
		IM_CHECK(_targetNormal.x != before.x || _targetNormal.y != before.y || _targetNormal.z != before.z);

		ctx->ItemClick("##flipnormal/Y");
		ctx->Yield();
		ctx->ItemClick("##flipnormal/Z");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "rotate buttons")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));

		const glm::vec3 before = _targetNormal;
		ctx->ItemClick("##rotnormal/X");
		ctx->Yield();
		IM_CHECK(_targetNormal.x != before.x || _targetNormal.y != before.y || _targetNormal.z != before.z);

		ctx->ItemClick("##rotnormal/Y");
		ctx->Yield();
		ctx->ItemClick("##rotnormal/Z");
		ctx->Yield();
	};

	IM_REGISTER_TEST(engine, testCategory(), "direct normal input")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Normal palette", cfg::VoxEditShowNormalPalette));
		IM_CHECK(focusWindow(ctx, id));

		// InputFloat3 "Normal" has 3 sub-inputs accessible via $$0, $$1, $$2
		ctx->ItemInputValue("Normal/$$0", 1.0f);
		ctx->Yield();
		ctx->ItemInputValue("Normal/$$1", 0.0f);
		ctx->Yield();
		ctx->ItemInputValue("Normal/$$2", 0.0f);
		ctx->Yield();
	};
}

} // namespace voxedit
