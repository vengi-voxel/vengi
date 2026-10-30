/**
 * @file
 */

#include "ScriptManager.h"
#include "BrushRegistry.h"
#include "app/App.h"
#include "core/Log.h"
#include "io/Filesystem.h"

namespace voxedit {

void ScriptManager::construct() {
}

bool ScriptManager::init() {
	discoverBrushScripts();
	discoverSelectionModeScripts();
	return true;
}

void ScriptManager::shutdown() {
	clearBrushScripts();
	clearSelectionModeScripts();
}

void ScriptManager::setSelectBrush(SelectBrush *selectBrush) {
	_selectBrush = selectBrush;
}

void ScriptManager::discoverBrushScripts() {
	const io::FilesystemPtr &filesystem = io::filesystem();
	core::DynamicArray<io::FilesystemEntry> entities;
	filesystem->list("brushes", entities, "*.lua");
	for (const auto &e : entities) {
		auto brush = core::make_shared<LUABrush>(filesystem);
		if (!brush->init()) {
			Log::error("Failed to initialize lua brush");
			continue;
		}
		if (!brush->loadScript(e.name)) {
			Log::warn("Failed to load brush script: %s", e.name.c_str());
			brush->shutdown();
			continue;
		}
		if (_registry) {
			const core::String id = "script/" + e.name;
			_registry->add(id, brush);
			auto &presentation = _registry->presentation(id);
			presentation.label = brush->scriptName();
			presentation.icon = brush->iconString();
			presentation.panel = "script";
		}
		_luaBrushOwners.push_back(brush);
		_luaBrushes.push_back(brush.get());
		Log::debug("Discovered brush script: %s", e.name.c_str());
	}
	if (!_luaBrushes.empty()) {
		_activeLuaBrushIndex = 0;
	}
}

void ScriptManager::clearBrushScripts() {
	for (LUABrush *b : _luaBrushes) {
		b->shutdown();
		if (_registry) {
			_registry->remove(b);
	}
	}
	_luaBrushes.clear();
	_luaBrushOwners.clear();
	_activeLuaBrushIndex = -1;
}

void ScriptManager::reloadBrushScripts() {
	if (_beforeScriptChange) {
		_beforeScriptChange();
	}
	clearBrushScripts();
	discoverBrushScripts();
	Log::debug("Reloaded brush scripts (%i found)", (int)_luaBrushes.size());
}

void ScriptManager::discoverSelectionModeScripts() {
	const io::FilesystemPtr &filesystem = io::filesystem();
	core::DynamicArray<io::FilesystemEntry> entities;
	filesystem->list("selectionmodes", entities, "*.lua");
	for (const auto &e : entities) {
		LUASelectionMode *mode = new LUASelectionMode(filesystem);
		if (!mode->init()) {
			Log::error("Failed to initialize lua selection mode");
			delete mode;
			continue;
		}
		if (!mode->loadScript(e.name)) {
			Log::warn("Failed to load selection mode script: %s", e.name.c_str());
			mode->shutdown();
			delete mode;
			continue;
		}
		_luaSelectionModes.push_back(mode);
		Log::debug("Discovered selection mode script: %s", e.name.c_str());
	}
}

void ScriptManager::clearSelectionModeScripts() {
	for (LUASelectionMode *m : _luaSelectionModes) {
		m->shutdown();
		delete m;
	}
	_luaSelectionModes.clear();
}

void ScriptManager::reloadSelectionModeScripts() {
	if (_beforeScriptChange) {
		_beforeScriptChange();
	}
	// Invalidate the active lua selection mode before deleting the old scripts
	// to avoid dangling pointers in SelectBrush
	if (_selectBrush) {
		_selectBrush->setLuaSelectionMode(-1, nullptr);
	}
	clearSelectionModeScripts();
	discoverSelectionModeScripts();
	Log::debug("Reloaded selection mode scripts (%i found)", (int)_luaSelectionModes.size());
}

LUABrush *ScriptManager::activeLuaBrush() {
	if (_activeLuaBrushIndex >= 0 && _activeLuaBrushIndex < (int)_luaBrushes.size()) {
		return _luaBrushes[_activeLuaBrushIndex];
	}
	return nullptr;
}

const LUABrush *ScriptManager::activeLuaBrush() const {
	if (_activeLuaBrushIndex >= 0 && _activeLuaBrushIndex < (int)_luaBrushes.size()) {
		return _luaBrushes[_activeLuaBrushIndex];
	}
	return nullptr;
}

void ScriptManager::setActiveLuaBrushIndex(int index) {
	if (_beforeScriptChange) {
		_beforeScriptChange();
	}
	if (index >= 0 && index < (int)_luaBrushes.size()) {
		_activeLuaBrushIndex = index;
	}
}

} // namespace voxedit
