/**
 * @file
 */

#include "BrushPanel.h"
#include "BrushPanelCommon.h"
#include "ScopedStyle.h"
#include "Style.h"
#include "Toolbar.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "command/CommandHandler.h"
#include "core/Bits.h"
#include "core/Enum.h"
#include "core/Trace.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "voxedit-ui/ViewMode.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/ScriptManager.h"
#include "voxedit-util/modifier/brush/BrushType.h"
#include "voxedit-util/modifier/brush/LUABrush.h"

namespace voxedit {

void BrushPanel::init() {
	_ctx.renderNormals = core::getVar(cfg::RenderNormals);
	_ctx.viewMode = core::getVar(cfg::VoxEditViewMode);
}

float BrushPanel::toolbarDockHeight() const {
	ui::ScopedStyle style;
	style.pushFontSize(imguiApp()->bigFontSize());
	const ImGuiStyle &imguiStyle = ImGui::GetStyle();
	const float rowHeight = ImGui::GetFrameHeightWithSpacing();
	const float paddingY = imguiStyle.WindowPadding.y * 2.0f;
	const float separatorHeight =
		imguiStyle.ItemSpacing.y + imguiStyle.SeparatorTextBorderSize + imguiStyle.SeparatorTextPadding.y * 2.0f;
	// Brush icons may wrap on a narrow left column; reserve two rows plus modifier row.
	static constexpr int MaxBrushToolbarRows = 2;
	static constexpr int ModifierToolbarRows = 1;
	return paddingY + MaxBrushToolbarRows * rowHeight + separatorHeight + ModifierToolbarRows * rowHeight;
}

void BrushPanel::brushSettings(command::CommandExecutionListener &listener) {
	const Modifier &modifier = _ctx.sceneMgr->modifier();
	const BrushType brushType = modifier.brushType();
	if (brushType == BrushType::None && !modifier.isMode(ModifierType::ColorPicker)) {
		return;
	}
	if (brushType == BrushType::Shape) {
		_shape.update(_ctx, listener);
	} else if (brushType == BrushType::Stamp) {
		_stamp.update(_ctx, listener);
	} else if (brushType == BrushType::Plane) {
		_plane.update(_ctx, listener);
	} else if (brushType == BrushType::Line) {
		_line.update(_ctx, listener);
	} else if (brushType == BrushType::Paint) {
		_paint.update(_ctx, listener);
	} else if (brushType == BrushType::Text) {
		_text.update(_ctx, listener);
	} else if (brushType == BrushType::Select) {
		_select.update(_ctx, listener);
	} else if (brushType == BrushType::Texture) {
		_texture.update(_ctx, listener);
	} else if (brushType == BrushType::Normal) {
		_normal.update(_ctx, listener);
	} else if (brushType == BrushType::Extrude) {
		_extrude.update(_ctx, listener);
	} else if (brushType == BrushType::Transform) {
		_transform.update(_ctx, listener);
	} else if (brushType == BrushType::Sculpt) {
		_sculpt.update(_ctx, listener);
	} else if (brushType == BrushType::Ruler) {
		_ruler.update(_ctx, listener);
	} else if (brushType == BrushType::Script) {
		_script.update(_ctx, listener);
	}

	if (modifier.isMode(ModifierType::ColorPicker)) {
		ImGui::TextWrappedUnformatted(_("Click on a voxel to pick the color"));
	}
}

void BrushPanel::addModifiers(command::CommandExecutionListener &listener) {
	ui::ScopedStyle style;
	style.pushFontSize(imguiApp()->bigFontSize());

	voxedit::Modifier &modifier = _ctx.sceneMgr->modifier();
	const BrushType brushType = modifier.brushType();
	const bool normalPaletteMode = viewModeNormalPalette(_ctx.viewMode->intVal());

	ui::Toolbar toolbarBrush("brushes", &listener);
	for (int i = 0; i < (int)BrushType::Max; ++i) {
		if (i == (int)BrushType::Normal && !normalPaletteMode) {
			continue;
		}
		if (i == (int)BrushType::Script) {
			continue;
		}
		core::String cmd = core::String::format("brush%s", BrushTypeStr[i]).toLower();
		auto func = [&listener, cmd]() { command::executeCommands(cmd, &listener); };
		core::String tooltip = command::help(cmd);
		if (tooltip.empty()) {
			tooltip = BrushTypeStr[i];
		}
		const bool currentBrush = (int)brushType == i;
		ui::ScopedStyle styleButton;
		if (currentBrush) {
			styleButton.setButtonColor(style::color(style::ColorActiveBrush));
		}
		toolbarBrush.button(BrushTypeIcons[i], tooltip.c_str(), func, !currentBrush);
	}

	// Render per-script brush buttons
	ScriptManager &scriptMgr = modifier.scriptManager();
	const core::DynamicArray<LUABrush *> &luaBrushes = scriptMgr.luaBrushes();
	const int activeLuaIdx = scriptMgr.activeLuaBrushIndex();
	for (int i = 0; i < (int)luaBrushes.size(); ++i) {
		const LUABrush *lb = luaBrushes[i];
		const bool isActive = brushType == BrushType::Script && activeLuaIdx == i;
		ui::ScopedStyle styleButton;
		if (isActive) {
			styleButton.setButtonColor(style::color(style::ColorActiveBrush));
		}
		const int idx = i;
		auto func = [&modifier, &scriptMgr, idx]() {
			modifier.setBrushType(BrushType::Script);
			scriptMgr.setActiveLuaBrushIndex(idx);
		};
		toolbarBrush.button(lb->iconString(), lb->scriptName().c_str(), func, !isActive);
	}

	toolbarBrush.end();

	ImGui::Separator();

	const ModifierType supported = modifier.checkModifierType();
	if (core::countSetBits(core::enumVal(supported)) > 1) {
		ui::Toolbar toolbarModifiers("modifiers", &listener);
		if ((supported & ModifierType::ColorPicker) != ModifierType::None) {
			toolbarModifiers.button(ICON_LC_PIPETTE, "actioncolorpicker", !modifier.isMode(ModifierType::ColorPicker));
		}
		if ((supported & ModifierType::Place) != ModifierType::None) {
			toolbarModifiers.button(ICON_LC_BOX, "actionplace", !modifier.isMode(ModifierType::Place));
		}
		if ((supported & ModifierType::Erase) != ModifierType::None) {
			toolbarModifiers.button(ICON_LC_ERASER, "actionerase", !modifier.isMode(ModifierType::Erase));
		}
		if ((supported & ModifierType::Override) != ModifierType::None) {
			toolbarModifiers.button(ICON_LC_SQUARE_PEN, "actionoverride", !modifier.isMode(ModifierType::Override));
		}
	} else {
		modifier.setModifierType(supported);
	}
}

void BrushPanel::createPopups(command::CommandExecutionListener &listener) {
	_texture.createPopups(_ctx, listener);
}

void BrushPanel::updateToolbar(const char *id, bool sceneMode, command::CommandExecutionListener &listener) {
	core_trace_scoped(BrushPanelToolbar);
	float minHeight = toolbarDockHeight();
	if (sceneMode) {
		minHeight += ImGui::GetTextLineHeightWithSpacing() * 3.0f;
	}
	ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, minHeight), ImVec2(FLT_MAX, minHeight));
	const core::String title = makeTitle(ICON_LC_BRUSH, _("Brushes"), id);
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoScrollbar;
	if (ImGui::Begin(title.c_str(), nullptr, flags)) {
		if (sceneMode) {
			ImGui::TextWrappedUnformatted(
				_("Brushes are only available in edit mode - you are currently in scene mode"));
		} else {
			addModifiers(listener);
		}
	}
	ImGui::End();
}

void BrushPanel::updateSettings(const char *id, bool sceneMode, command::CommandExecutionListener &listener) {
	core_trace_scoped(BrushPanelSettings);
	const core::String title = makeTitle(ICON_LC_BRUSH, _("Brush settings"), id);
	if (ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (sceneMode) {
			ImGui::TextWrappedUnformatted(
				_("Brushes are only available in edit mode - you are currently in scene mode"));
		} else {
			brushSettings(listener);
			createPopups(listener);
		}
	}
	ImGui::End();
}

} // namespace voxedit
