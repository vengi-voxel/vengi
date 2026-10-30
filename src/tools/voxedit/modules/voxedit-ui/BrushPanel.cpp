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
#include "ui/ScopedPanel.h"
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
	_companions.clear();
	auto &registry = _ctx.sceneMgr->modifier().registry();
	for (const Brush *tool : registry.tools()) {
		auto &presentation = registry.presentation(registry.id(*tool));
		if (presentation.icon.empty()) {
			presentation.icon = BrushTypeIcons[(int)tool->type()];
			presentation.command = "brush" + tool->name().toLower();
			presentation.normalPaletteOnly = tool->volumeModifier(ModifierType::None) == ModifierType::NormalPaint;
		}
	}
	registerPanel<BrushPanelShape>("shape");
	registerPanel<BrushPanelStamp>("stamp");
	registerPanel<BrushPanelPlane>("plane");
	registerPanel<BrushPanelLine>("line");
	registerPanel<BrushPanelPaint>("paint");
	registerPanel<BrushPanelText>("text");
	registerPanel<BrushPanelSelect>("select");
	registerPanel<BrushPanelNormal>("normal");
	registerPanel<BrushPanelExtrude>("extrude");
	registerPanel<BrushPanelTransform>("transform");
	registerPanel<BrushPanelSculpt>("sculpt");
	registerPanel<BrushPanelRuler>("ruler");
	registerPanel<BrushPanelScript>("script");
	auto texture = registerPanel<BrushPanelTexture>("texture");
	_companions.back().popups = [texture](BrushPanelContext &ctx, command::CommandExecutionListener &listener) {
		texture->createPopups(ctx, listener);
	};
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
	Modifier &modifier = _ctx.sceneMgr->modifier();
	if (!modifier.currentBrush() && !modifier.isMode(ModifierType::ColorPicker)) {
		return;
	}
	for (const Companion &companion : _companions) {
		if (modifier.currentBrush() &&
			companion.id == modifier.registry().presentation(modifier.registry().id(*modifier.currentBrush())).panel) {
			companion.update(_ctx, listener);
			break;
		}
	}
	modifier.flushPendingBrushChanges();

	if (modifier.operationError()) {
		ImGui::TextWrappedUnformatted(_(modifier.operationError()));
	}
	if (modifier.isAdjustable() && modifier.hasDraft()) {
		ImGui::BeginDisabled(modifier.operationError() != nullptr);
		if (ImGui::ButtonFullWidth(_("Apply"))) {
			_ctx.sceneMgr->modifier().applyDraft();
	}
		ImGui::EndDisabled();
		if (ImGui::ButtonFullWidth(_("Cancel"))) {
			_ctx.sceneMgr->modifier().cancelDraft();
	}
	}

	if (modifier.isMode(ModifierType::ColorPicker)) {
		ImGui::TextWrappedUnformatted(_("Click on a voxel to pick the color"));
	}
}

void BrushPanel::addModifiers(command::CommandExecutionListener &listener) {
	ui::ScopedStyle style;
	style.pushFontSize(imguiApp()->bigFontSize());

	voxedit::Modifier &modifier = _ctx.sceneMgr->modifier();
	const bool normalPaletteMode = core::getVar(cfg::VoxEditShowNormalPalette)->boolVal();

	ui::Toolbar toolbarBrush("brushes", &listener);
	const auto &registry = modifier.registry();
	for (const Brush *tool : registry.tools()) {
		const core::String id = registry.id(*tool);
		const auto &presentation = registry.presentation(id);
		if (presentation.normalPaletteOnly && !normalPaletteMode) {
			continue;
		}
		const bool active = modifier.currentBrush() == tool;
		ui::ScopedStyle styleButton;
		if (active) {
			styleButton.setButtonColor(style::color(style::ColorActiveBrush));
		}
		const core::String tooltip =
			presentation.command.empty() ? presentation.label : command::help(presentation.command);
		auto activate = [&modifier, id]() {
			modifier.setTool(id);
		};
		const char *icon = presentation.icon.empty() ? ICON_LC_BRUSH : presentation.icon.c_str();
		toolbarBrush.button(icon, tooltip.empty() ? presentation.label.c_str() : tooltip.c_str(), activate, !active);
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
	for (const Companion &companion : _companions) {
		if (companion.popups) {
			companion.popups(_ctx, listener);
		}
	}
}

void BrushPanel::updateToolbar(const char *id, bool sceneMode, command::CommandExecutionListener &listener) {
	core_trace_scoped(BrushPanelToolbar);
	static ui::ScopedPanel panel(cfg::VoxEditShowBrushes);
	if (!panel.isOpen()) {
		return;
	}
	const core::String &title = makeTitle(ICON_LC_BRUSH, _("Brushes"), id);
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoFocusOnAppearing;
	if (ui::ScopedPanel::Scope scope = panel.begin(title.c_str(), flags)) {
		if (sceneMode) {
			ImGui::TextWrappedUnformatted(
				_("Brushes are only available in edit mode - you are currently in scene mode"));
		} else {
			addModifiers(listener);
		}
	}
}

void BrushPanel::updateSettings(const char *id, bool sceneMode, command::CommandExecutionListener &listener) {
	core_trace_scoped(BrushPanelSettings);
	static ui::ScopedPanel panel(cfg::VoxEditShowBrushes);
	if (!panel.isOpen()) {
		return;
	}
	const core::String &title = makeTitle(ICON_LC_BRUSH, _("Brush settings"), id);
	if (ui::ScopedPanel::Scope scope = panel.begin(title.c_str(), ImGuiWindowFlags_NoFocusOnAppearing)) {
		if (sceneMode) {
			ImGui::TextWrappedUnformatted(
				_("Brushes are only available in edit mode - you are currently in scene mode"));
		} else {
			brushSettings(listener);
			createPopups(listener);
		}
	}
}

} // namespace voxedit
