/**
 * @file
 */

#include "LUAUIApp.h"
#include "LUAFunctions.h"
#include "core/io/Filesystem.h"
#include "core/Log.h"
#include "core/command/Command.h"
#include "core/Trace.h"
#include "Nuklear.h"

namespace ui {
namespace nuklear {

LUAUIApp::LUAUIApp(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const video::TexturePoolPtr& texturePool) :
		Super(metric, filesystem, eventBus, timeProvider), _lua(false), _texturePool(texturePool) {
}

LUAUIApp::~LUAUIApp() {
}

core::AppState LUAUIApp::onInit() {
	const core::AppState state = Super::onInit();

	if (!_texturePool->init()) {
		return core::AppState::InitFailure;
	}

	if (!reload()) {
		return core::AppState::InitFailure;
	}

	const std::string uiScriptPath = "ui/" + appname() + ".lua";
	const io::FilesystemPtr& fs = filesystem();
	if (!fs->watch(uiScriptPath, [] (const char *name) {
		Log::info("Reload ui script: '%s'", name);
		LUAUIApp* app = (LUAUIApp*)core::App::getInstance();
		app->reload();
	})) {
		Log::warn("Failed to install file watcher");
	} else {
		Log::info("Installed file watcher for '%s'", uiScriptPath.c_str());
	}

	core::Command::registerCommand("uireload", [this] (const core::CmdArgs&) {reload();});

	return state;
}

core::AppState LUAUIApp::onCleanup() {
	_texturePool->shutdown();
	return Super::onCleanup();
}

bool LUAUIApp::onRenderUI() {
	if (_skipUntilReload) {
		return true;
	}
	core_trace_scoped(LUAAIAppOnRenderUI);
	if (!_lua.executeUpdate(_deltaFrameMillis)) {
		Log::error("LUA UI: %s", _lua.error().c_str());
		_skipUntilReload = true;
		return false;
	}
	return true;
}

bool LUAUIApp::reload() {
	const bool console = _console.isActive();
	if (console) {
		_console.toggle();
	}
	_skipUntilReload = true;
	core_assert_always(_lua.resetState());

	const std::vector<luaL_Reg> funcs = {
		{"windowBegin", uilua_window_begin},
		{"windowEnd", uilua_window_end},
		{"getWindowBounds", uilua_window_get_bounds},
		{"getWindowPos", uilua_window_get_position},
		{"getWindowSize", uilua_window_get_size},
		{"getWindowContentRegion", uilua_window_get_content_region},

		{"edit", uilua_edit},
		{"text", uilua_text},
		{"label", uilua_label},
		{"image", uilua_image},
		{"checkbox", uilua_checkbox},
		{"radio", uilua_radio},
		{"selectable", uilua_selectable},
		{"slider", uilua_slider},
		{"progress", uilua_progress},
		{"colorpicker", uilua_color_picker},
		{"property", uilua_property},

		{"button", uilua_button},
		{"buttonSetBehaviour", uilua_button_set_behavior},
		{"buttonPushBehaviour", uilua_button_push_behavior},
		{"buttonPopBehaviour", uilua_button_pop_behavior},

		{"hasWindowFocus", uilua_window_has_focus},
		{"isWindowCollapsed", uilua_window_is_collapsed},
		{"isWindowHidden", uilua_window_is_hidden},
		{"isWindowActive", uilua_window_is_active},
		{"isWindowHovered", uilua_window_is_hovered},
		{"isAnyWindowHovered", uilua_window_is_any_hovered},
		{"isAnythingActive", uilua_item_is_any_active},
		{"setWindowBounds", uilua_window_set_bounds},
		{"setWindowPosition", uilua_window_set_position},
		{"setWindowSize", uilua_window_set_size},
		{"setWindowFocus", uilua_window_set_focus},
		{"windowClose", uilua_window_close},
		{"windowCollapse", uilua_window_collapse},
		{"windowExpand", uilua_window_expand},
		{"windowShow", uilua_window_show},
		{"windowHide", uilua_window_hide},

		{"layoutRow", uilua_layout_row},
		{"layoutRowBegin", uilua_layout_row_begin},
		{"layoutRowPush", uilua_layout_row_push},
		{"layoutRowEnd", uilua_layout_row_end},
		{"layoutSpaceBegin", uilua_layout_space_begin},
		{"layoutSpacePush", uilua_layout_space_push},
		{"layoutSpaceEnd", uilua_layout_space_end},
		{"getLayoutSpaceBounds", uilua_layout_space_bounds},
		{"layoutSpaceToScreen", uilua_layout_space_to_screen},
		{"layoutSpaceToLocal", uilua_layout_space_to_local},
		{"layoutSpaceRectToScreen", uilua_layout_space_rect_to_screen},
		{"layoutSpaceRectToLocal", uilua_layout_space_rect_to_local},
		{"layoutSpaceRatioFromPixel", uilua_layout_ratio_from_pixel},

		{"groupBegin", uilua_group_begin},
		{"groupEnd", uilua_group_end},

		{"treePush", uilua_tree_push},
		{"treePop", uilua_tree_pop},

		{"popupBegin", uilua_popup_begin},
		{"popupClose", uilua_popup_close},
		{"popupEnd", uilua_popup_end},

		{"combobox", uilua_combobox},
		{"comboboxBegin", uilua_combobox_begin},
		{"comboboxItem", uilua_combobox_item},
		{"comboboxClose", uilua_combobox_close},
		{"comboboxEnd", uilua_combobox_end},

		{"contextualBegin", uilua_contextual_begin},
		{"contextualItem", uilua_contextual_item},
		{"contextualClose", uilua_contextual_close},
		{"contextualEnd", uilua_contextual_end},

		{"tooltip", uilua_tooltip},
		{"tooltipBegin", uilua_tooltip_begin},
		{"tooltipEnd", uilua_tooltip_end},

		{"menubarBegin", uilua_menubar_begin},
		{"menubarEnd", uilua_menubar_end},

		{"menuBegin", uilua_menu_begin},
		{"menuItem", uilua_menu_item},
		{"menuClose", uilua_menu_close},
		{"menuEnd", uilua_menu_end},

		{"getWidgetBounds", uilua_widget_bounds},
		{"getWidgetPosition", uilua_widget_position},
		{"getWidgetSize", uilua_widget_size},
		{"getWidgetWidth", uilua_widget_width},
		{"getWidgetHeight", uilua_widget_height},
		{"isWidgetHovered", uilua_widget_is_hovered},

		{"spacing", uilua_spacing},

		{"scissor", uilua_push_scissor},

		{nullptr, nullptr}
	};
	_lua.newGlobalData<struct nk_context>("context", &_ctx);
	_lua.newGlobalData<video::TexturePool>("texturepool", _texturePool.get());
	_lua.reg("ui", &funcs.front());

	const io::FilesystemPtr& fs = filesystem();
	const std::string uiScriptPath = "ui/" + appname() + ".lua";
	const std::string& luaScript = fs->load(uiScriptPath);
	if (luaScript.empty()) {
		Log::error("Could not load ui script from '%s'", uiScriptPath.c_str());
		return false;
	}
	if (!_lua.load(luaScript)) {
		Log::error("Could not execute lua script from '%s': %s", uiScriptPath.c_str(), _lua.error().c_str());
		return false;
	}
	_skipUntilReload = false;
	return true;
}

}
}
