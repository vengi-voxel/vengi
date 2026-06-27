/**
 * @file
 */

#include "ViewportHud.h"
#include "BrushPanelCommon.h"
#include "app/I18N.h"
#include "core/ArrayLength.h"
#include "ui/IMGUIEx.h"
#include "ui/Style.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/ExtrudeBrush.h"
#include "voxedit-util/modifier/brush/LUABrush.h"
#include "voxedit-util/modifier/brush/SculptBrush.h"
#include "voxel/Face.h"

namespace voxedit {
namespace viewporthud {

namespace {

struct Line {
	const char *text;
	ImU32 color;
};

static ImU32 imguiThemeColor(ImGuiCol idx) {
	return ImGui::GetColorU32(idx);
}

static ImU32 styleThemeColor(style::StyleColor color) {
	return ImGui::GetColorU32(ImVec4(style::color(color)));
}

static const char *modifierTypeLabel(ModifierType type) {
	if (type == ModifierType::ColorPicker) {
		return _("Color picker");
	}
	if (type == ModifierType::Place) {
		return _("Place");
	}
	if (type == ModifierType::Erase) {
		return _("Erase");
	}
	if (type == ModifierType::Override) {
		return _("Override");
	}
	if (type == ModifierType::Paint) {
		return _("Paint");
	}
	if (type == ModifierType::NormalPaint) {
		return _("Normal paint");
	}
	return nullptr;
}

static void brushTitle(Modifier &modifier, core::String &title) {
	const BrushType brushType = modifier.brushType();
	if (brushType == BrushType::None) {
		title = _("No brush");
		return;
	}
	const int idx = (int)brushType;
	if (brushType == BrushType::Script) {
		const LUABrush *luaBrush = modifier.scriptManager().activeLuaBrush();
		if (luaBrush != nullptr) {
			title = core::String::format("%s %s", BrushTypeIcons[idx], luaBrush->scriptName().c_str());
		} else {
			title = core::String::format("%s %s", BrushTypeIcons[idx], BrushTypeStr[idx]);
		}
		return;
	}
	title = core::String::format("%s %s", BrushTypeIcons[idx], _(BrushTypeStr[idx]));
}

static void appendHint(const SceneManagerPtr &sceneMgr, Modifier &modifier, Line *lines, int &lineCount,
					   const int maxLines, ImU32 hintColor, ImU32 warningColor) {
	if (lineCount >= maxLines) {
		return;
	}
	const BrushType brushType = modifier.brushType();
	const int nodeId = sceneMgr->sceneGraph().activeNode();

	if (modifier.isMode(ModifierType::ColorPicker)) {
		lines[lineCount++] = {_("Click on a voxel to pick the color"), hintColor};
		return;
	}

	const Brush *brush = modifier.currentBrush();
	if (brush != nullptr && !brush->errorReason().empty()) {
		lines[lineCount++] = {brush->errorReason().c_str(), warningColor};
		return;
	}

	if (brushType == BrushType::Extrude) {
		const ExtrudeBrush &extrude = modifier.extrudeBrush();
		if (extrude.face() == voxel::FaceNames::Max) {
			lines[lineCount++] = {_("Click a voxel face to set extrusion direction"), warningColor};
		} else if (extrude.depth() == 0 && !sceneMgr->hasSelection(nodeId)) {
			lines[lineCount++] = {_("Select voxels first (Select brush)"), warningColor};
		}
		return;
	}

	if (brushType == BrushType::Transform && !sceneMgr->hasSelection(nodeId)) {
		lines[lineCount++] = {_("Select voxels first (Select brush)"), warningColor};
		return;
	}

	if (brushType == BrushType::Sculpt) {
		const SculptBrush &sculpt = modifier.sculptBrush();
		if (!sculpt.hasSnapshot() && !sceneMgr->hasSelection(nodeId)) {
			lines[lineCount++] = {_("Select voxels, then switch to Sculpt"), warningColor};
			return;
		}
		if (SculptBrush::modeNeedsFace(sculpt.sculptMode()) && sculpt.flattenFace() == voxel::FaceNames::Max) {
			lines[lineCount++] = {_("Click a voxel face to set direction"), warningColor};
		}
	}
}

static bool beginHudWindow(const ImVec2 &windowPos, const ImVec2 &contentSize, float headerSize) {
	const float padding = ImGui::GetStyle().FramePadding.x;
	const ImVec2 pos(windowPos.x + padding, windowPos.y + headerSize + contentSize.y - padding);
	ImGui::SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.0f, 1.0f));
	ImGui::SetNextWindowBgAlpha(0.9f);
	const ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration |
								   ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoSavedSettings |
								   ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoMove;
	return ImGui::Begin("##viewporthud", nullptr, flags);
}

static void renderAddNodeHud() {
	ImGui::TextUnformatted(_("Add node by face"));
	ImGui::CheckboxVar(cfg::VoxEditAddNodeIgnoreOverlap);
	ImGui::CheckboxVar(cfg::VoxEditAddNodeCloneVoxels);
	ImGui::TextDisabled("%s", _("Click a face to place, Esc to cancel"));
}

static void renderBrushHud(const SceneManagerPtr &sceneMgr) {
	Modifier &modifier = sceneMgr->modifier();
	const BrushType brushType = modifier.brushType();
	if (brushType == BrushType::None && !modifier.isMode(ModifierType::ColorPicker)) {
		return;
	}

	const ImU32 textColor = imguiThemeColor(ImGuiCol_Text);
	const ImU32 secondaryColor = imguiThemeColor(ImGuiCol_TextDisabled);
	const ImU32 warningColor = styleThemeColor(style::ColorWarningText);
	const ImU32 titleColor = brushType == BrushType::None ? textColor : styleThemeColor(style::ColorActiveBrush);

	Line lines[4];
	int lineCount = 0;

	core::String title;
	brushTitle(modifier, title);
	lines[lineCount++] = {title.c_str(), titleColor};

	const char *modLabel = modifierTypeLabel(modifier.modifierType());
	if (modLabel != nullptr && brushType != BrushType::None) {
		lines[lineCount++] = {modLabel, secondaryColor};
	}

	appendHint(sceneMgr, modifier, lines, lineCount, (int)lengthof(lines), secondaryColor, warningColor);

	for (int i = 0; i < lineCount; ++i) {
		if (i == 0) {
			ImGui::PushStyleColor(ImGuiCol_Text, lines[i].color);
			ImGui::TextUnformatted(lines[i].text);
			ImGui::PopStyleColor();
		} else if (lines[i].color == warningColor) {
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(lines[i].color), "%s", lines[i].text);
		} else {
			ImGui::TextDisabled("%s", lines[i].text);
		}
	}
}

} // namespace

void render(const SceneManagerPtr &sceneMgr, bool sceneMode, const ImVec2 &windowPos, const ImVec2 &contentSize,
			float headerSize) {
	if (sceneMode) {
		if (!sceneMgr->isAddNodeModeActive()) {
			return;
		}
		if (!beginHudWindow(windowPos, contentSize, headerSize)) {
			ImGui::End();
			return;
		}
		renderAddNodeHud();
		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
			sceneMgr->setViewportHudHovered(true);
		}
		ImGui::End();
		return;
	}

	Modifier &modifier = sceneMgr->modifier();
	const BrushType brushType = modifier.brushType();
	if (brushType == BrushType::None && !modifier.isMode(ModifierType::ColorPicker)) {
		return;
	}
	if (!beginHudWindow(windowPos, contentSize, headerSize)) {
		ImGui::End();
		return;
	}
	renderBrushHud(sceneMgr);
	if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
		sceneMgr->setViewportHudHovered(true);
	}
	ImGui::End();
}

} // namespace viewporthud
} // namespace voxedit
