/**
 * @file
 */

#include "LUAUIApp.h"
#include "LUAFunctions.h"
#include "commonlua/LUAFunctions.h"
#include "core/io/Filesystem.h"
#include "core/Log.h"
#include "core/command/Command.h"
#include "core/Trace.h"

namespace ui {
namespace nuklear {

LUAUIApp::LUAUIApp(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const video::TexturePoolPtr& texturePool,
		const voxelrender::CachedMeshRendererPtr& meshRenderer,
		const video::TextureAtlasRendererPtr& textureAtlasRenderer) :
		Super(metric, filesystem, eventBus, timeProvider, texturePool, meshRenderer, textureAtlasRenderer), _lua(false) {
}

LUAUIApp::~LUAUIApp() {
}

core::AppState LUAUIApp::onInit() {
	const core::AppState state = Super::onInit();

	if (!_texturePool->init()) {
		return core::AppState::InitFailure;
	}

	const core::String& path = core::string::format("ui/%s.lua", appname().c_str());
	_uiScriptPath = core::Var::get("ui_script", path)->strVal();

	if (!reload()) {
		return core::AppState::InitFailure;
	}

	core::Command::registerCommand("ui_reload", [this] (const core::CmdArgs&) {
		reload();
	});

	core::Command::registerCommand("ui_pop", [this] (const core::CmdArgs&) {
		popWindow();
	});

	core::Command::registerCommand("ui_push", [this] (const core::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: ui_push <windowid> <parameter>");
			return;
		}
		const core::String& window = args[0];
		if (args.size() == 2) {
			pushWindow(window, args[1]);
		} else {
			if (args.size() > 2) {
				Log::info("Ignoring parameters");
			}
			pushWindow(window);
		}
	});

	core::Command::registerCommand("ui_stack", [this] (const core::CmdArgs& args) {
		const size_t size = _windowStack.size();
		Log::info("Current window stack");
		for (size_t i = 0; i < size; ++i) {
			Log::info(" %i: %s ['%s']", (int)i, _windowStack[i].id.c_str(), _windowStack[i].parameter.c_str());
		}
	});

	return state;
}

core::AppState LUAUIApp::onCleanup() {
	_texturePool->shutdown();
	return Super::onCleanup();
}

void LUAUIApp::popup(const core::String& message) {
	pushWindow("popup", message);
}

bool LUAUIApp::onRenderUI() {
	core_trace_scoped(LUAAIAppOnRenderUI);

	if (_windowStack.empty() && !_rootWindow.empty()) {
		pushWindow(_rootWindow);
	}

	lua_State* state = _lua.state();
	WindowStack copy(_windowStack);
	const size_t windowCount = copy.size();
	const size_t limit = _skipUntilReload >= 0 ? _skipUntilReload : windowCount;
	for (size_t i = 0u; i < limit; ++i) {
		const auto& window = copy[i];
		lua_getglobal(state, window.id.c_str());
		if (lua_isnil(state, -1)) {
			Log::error("window: %s: wasn't found", window.id.c_str());
			_skipUntilReload = i;
			break;
		}
		const int argc = window.parameter.empty() ? 0 : 1;
		if (argc == 1) {
			lua_pushstring(state, window.parameter.c_str());
		}
		const int ret = lua_pcall(state, argc, 0, 0);
		if (ret != LUA_OK) {
			Log::error("window: %s: execution error: %s", window.id.c_str(), lua_tostring(state, -1));
			_skipUntilReload = i;
			break;
		}
	}
	return true;
}

void LUAUIApp::setGlobalAlpha(float alpha) {
	_config.global_alpha = glm::clamp(alpha, 0.0f, 1.0f);
}

void LUAUIApp::rootWindow(const core::String& id) {
	popWindow(_windowStack.size());
	_rootWindow = id;
	Log::info("Root window %s", _rootWindow.c_str());
}

void LUAUIApp::pushWindow(const core::String& id, const core::String& parameter) {
	if (id.empty()) {
		return;
	}
	_windowStack.emplace(id, parameter);
	Log::info("Push window %s", id.c_str());
}

void LUAUIApp::popWindow(int amount) {
	for (int i = 0; i < amount; ++i) {
		if (_windowStack.size() == 0) {
			break;
		}
		Log::info("Pop window %s", _windowStack.top().id.c_str());
		_windowStack.pop();
	}
}

bool LUAUIApp::reload() {
	const bool console = _console.isActive();
	if (console) {
		_console.toggle();
	}

	core_assert_always(_lua.resetState());

	const luaL_Reg uiFuncs[] = {
		{"globalAlpha", uilua_global_alpha},
		{"rootWindow", uilua_window_root},
		{"windowPush", uilua_window_push},
		{"windowPop", uilua_window_pop},

		{"windowBegin", uilua_window_begin},
		{"windowEnd", uilua_window_end},
		{"getWindowBounds", uilua_window_get_bounds},
		{"getWindowPos", uilua_window_get_position},
		{"getWindowSize", uilua_window_get_size},
		{"getWindowContentRegion", uilua_window_get_content_region},

		{"model", uilua_model},
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

		{"styleDefault", uilua_style_default},
		{"styleLoadColors", uilua_style_load_colors},
		{"styleSetFont", uilua_style_set_font},
		{"stylePush", uilua_style_push},
		{"stylePop", uilua_style_pop},
		{"style", uilua_style},

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
	_lua.newGlobalData<struct nkc_context>("ccontext", &_cctx);
	_lua.newGlobalData<LUAUIApp>("app", this);
	_lua.newGlobalData<video::TexturePool>("texturepool", _texturePool.get());
	_lua.reg("ui", uiFuncs);

	lua_newtable(_lua.state());
	lua_setglobal(_lua.state(), "stack");

	clua_mathregister(_lua);

	configureLUA(_lua);

	const io::FilesystemPtr& fs = filesystem();
	const core::String& luaScript = fs->load(_uiScriptPath);
	if (luaScript.empty()) {
		Log::error("Could not load ui script from '%s'", _uiScriptPath.c_str());
		return false;
	}
	if (!_lua.load(luaScript)) {
		Log::error("Could not execute lua script from '%s': %s", _uiScriptPath.c_str(), _lua.error().c_str());
		return false;
	}
	_skipUntilReload = -1;
	return true;
}

}
}
