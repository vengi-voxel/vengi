/**
 * @file
 */

#include "LUAUIApp.h"
#include "LUAFunctions.h"
#include "io/Filesystem.h"
#include "core/Log.h"
#include "core/Trace.h"

namespace nuklear {

LUAUIApp::LUAUIApp(const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider, uint16_t traceport) :
		Super(filesystem, eventBus, timeProvider, traceport) {
}

LUAUIApp::~LUAUIApp() {
}

core::AppState LUAUIApp::onInit() {
	const core::AppState state = Super::onInit();

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

	return state;
}

void LUAUIApp::onRenderUI() {
	core_trace_scoped(LUAAIAppOnRenderUI);
	_lua.execute("update");
}

bool LUAUIApp::reload() {
	core_assert_always(_lua.resetState());

	const std::vector<luaL_Reg> funcs = {
		{"windowBegin", uilua_window_begin},
		{"windowEnd", uilua_window_end},
		{"getWindowBounds", uilua_window_get_bounds},
		{"getWindowPos", uilua_window_get_position},
		{"getWindowSize", uilua_window_get_size},
		{"getWindowContentRegion", uilua_window_get_content_region},
		{nullptr, nullptr}
	};
	_lua.newGlobalData<struct nk_context>("context", &_ctx);
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
	return true;
}

}
