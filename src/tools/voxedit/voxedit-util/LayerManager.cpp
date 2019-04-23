/**
 * @file
 */

#include "LayerManager.h"
#include "core/String.h"
#include "core/command/Command.h"

namespace voxedit {

void LayerManager::construct() {
	core::Command::registerCommand("layeradd", [&] (const core::CmdArgs& args) {
		const char *name = args.size() > 0 ? args[0].c_str() : "";
		const char *width = args.size() > 1 ? args[1].c_str() : "64";
		const char *height = args.size() > 2 ? args[2].c_str() : width;
		const char *depth = args.size() > 3 ? args[3].c_str() : height;
		const int iw = core::string::toInt(width) - 1;
		const int ih = core::string::toInt(height) - 1;
		const int id = core::string::toInt(depth) - 1;
		const voxel::Region region(glm::zero<glm::ivec3>(), glm::ivec3(iw, ih, id));
		if (!region.isValid()) {
			Log::warn("Invalid size provided (%i:%i:%i - %s:%s:%s)", iw, ih, id, width, height, depth);
			return;
		}
		voxel::RawVolume* v = new voxel::RawVolume(region);
		const int layerId = addLayer(name, true, v);
		if (layerId >= 0) {
			setActiveLayer(layerId);
		}
	}).setHelp("Add a new layer (with a given name and width, height, depth - all optional)");
	core::Command::registerCommand("layerdelete", [&] (const core::CmdArgs& args) {
		deleteLayer(args.size() > 0 ? core::string::toInt(args[0]) : activeLayer());
	}).setHelp("Delete a particular layer by id - or the current active one");
	core::Command::registerCommand("layeractive", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Active layer: %i", activeLayer());
		} else {
			const int newActiveLayer = core::string::toInt(args[0]);
			if (!setActiveLayer(newActiveLayer)) {
				Log::warn("Failed to make %i the active layer", newActiveLayer);
			}
		}
	}).setHelp("Set or print the current active layer");
	core::Command::registerCommand("layerstate", [&] (const core::CmdArgs& args) {
		if (args.empty()) {
			Log::info("Usage: layerstate <layerid> <true|false>");
			return;
		}
		const int layerId = core::string::toInt(args[0]);
		const bool newVisibleState = core::string::toBool(args[1]);
		hideLayer(layerId, !newVisibleState);
	}).setHelp("Change the visible state of a layer");
	core::Command::registerCommand("layerhideall", [&] (const core::CmdArgs& args) {
		for (int idx = 0; idx < (int)_layers.size(); ++idx) {
			hideLayer(idx, true);
		}
	}).setHelp("Hide all layers");
	core::Command::registerCommand("layershowall", [&] (const core::CmdArgs& args) {
		for (int idx = 0; idx < (int)_layers.size(); ++idx) {
			hideLayer(idx, false);
		}
	}).setHelp("Show all layers");
}

bool LayerManager::init() {
	return true;
}

void LayerManager::shutdown() {
	_listeners.clear();
	_activeLayer = 0;
	const int size = (int)_layers.size();
	for (int i = 0; i < size; ++i) {
		_layers[i].reset();
	}
}

bool LayerManager::findNewActiveLayer() {
	_activeLayer = -1;
	const int size = (int)_layers.size();
	for (int i = 0; i < size; ++i) {
		if (_layers[i].valid && _activeLayer == -1) {
			if (setActiveLayer(i)) {
				return true;
			}
		}
	}
	_activeLayer = 0;
	return false;
}

void LayerManager::hideLayer(int layerId, bool hide) {
	for (auto& listener : _listeners) {
		if (hide) {
			listener->onLayerHide(layerId);
		} else {
			listener->onLayerShow(layerId);
		}
	}
}

int LayerManager::validLayers() const {
	int validLayers = 0;
	for (const auto& l : layers()) {
		if (!l.valid) {
			continue;
		}
		++validLayers;
	}
	return validLayers;
}

bool LayerManager::setActiveLayer(int layerId) {
	if (layerId < 0 || layerId >= (int)_layers.size()) {
		Log::debug("Given layer %i is out of bounds", layerId);
		return false;
	}
	if (!_layers[layerId].valid) {
		Log::debug("Given layer %i is not valid", layerId);
		return false;
	}
	Log::debug("New active layer: %i", layerId);
	int old = _activeLayer;
	_activeLayer = layerId;
	for (auto& listener : _listeners) {
		listener->onActiveLayerChanged(old, _activeLayer);
	}
	return true;
}

bool LayerManager::deleteLayer(int layerId, bool force) {
	if (layerId < 0 || layerId >= (int)_layers.size()) {
		Log::debug("Invalid layer id given: %i", layerId);
		return false;
	}
	if (!_layers[layerId].valid) {
		Log::debug("Deleting an invalid layer is a nop: %i", layerId);
		return true;
	}
	// don't delete the last layer
	if (!force && validLayers() == 1) {
		Log::debug("Can't delete last remaining layer: %i", layerId);
		return false;
	}
	const Layer oldLayer = _layers[layerId];
	_layers[layerId].reset();
	if (!force && layerId == activeLayer()) {
		core_assert_always(findNewActiveLayer());
	}
	for (auto& listener : _listeners) {
		listener->onLayerDeleted(layerId, oldLayer);
	}
	Log::debug("Layer %i was deleted", layerId);
	return true;
}

int LayerManager::addLayer(const char *name, bool visible, voxel::RawVolume* volume) {
	const size_t maxLayers = _layers.size();
	for (size_t layerId = 0; layerId < maxLayers; ++layerId) {
		if (_layers[layerId].valid) {
			continue;
		}
		activateLayer(layerId, name, visible, volume);
		return (int)layerId;
	}
	return -1;
}

bool LayerManager::activateLayer(int layerId, const char *name, bool visible, voxel::RawVolume* volume, const voxel::Region& region) {
	core_assert_always(layerId >= 0 && layerId < (int)_layers.size());
	if (name == nullptr || name[0] == '\0') {
		_layers[layerId].name = core::string::format("%i", (int)layerId);
	} else {
		_layers[layerId].name = name;
	}
	_layers[layerId].visible = visible;
	_layers[layerId].valid = volume != nullptr;
	for (auto& listener : _listeners) {
		listener->onLayerAdded((int)layerId, _layers[layerId], volume, region);
	}
	return true;
}

const Layer& LayerManager::layer(int layerId) const {
	core_assert_always(layerId >= 0 && layerId < (int)_layers.size());
	return _layers[layerId];
}

Layer& LayerManager::layer(int layerId) {
	core_assert_always(layerId >= 0 && layerId < (int)_layers.size());
	return _layers[layerId];
}

void LayerManager::registerListener(LayerListener* listener) {
	_listeners.insert(listener);
}

void LayerManager::unregisterListener(LayerListener* listener) {
	_listeners.erase(listener);
}

}
