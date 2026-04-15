/**
 * @file
 */

#include "ScriptManager.h"
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
		LUABrush *brush = new LUABrush(filesystem);
		if (!brush->init()) {
			Log::error("Failed to initialize lua brush");
			delete brush;
			continue;
		}
		if (!brush->loadScript(e.name)) {
			Log::warn("Failed to load brush script: %s", e.name.c_str());
			brush->shutdown();
			delete brush;
			continue;
		}
		_luaBrushes.push_back(brush);
		Log::debug("Discovered brush script: %s", e.name.c_str());
	}
	if (!_luaBrushes.empty()) {
		_activeLuaBrushIndex = 0;
	}
}

void ScriptManager::clearBrushScripts() {
	for (LUABrush *b : _luaBrushes) {
		b->shutdown();
		delete b;
	}
	_luaBrushes.clear();
	_activeLuaBrushIndex = -1;
}

void ScriptManager::reloadBrushScripts() {
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
	if (index >= 0 && index < (int)_luaBrushes.size()) {
		_activeLuaBrushIndex = index;
	}
}

} // namespace voxedit
