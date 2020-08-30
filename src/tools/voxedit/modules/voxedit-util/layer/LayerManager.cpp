/**
 * @file
 */

#include "LayerManager.h"
#include "core/StringUtil.h"
#include "command/Command.h"
#include "voxelutil/VolumeMerger.h"
#include "core/Log.h"
#include "core/Trace.h"
#include <algorithm>

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

	core::Command::registerCommand("layerlock", [&] (const core::CmdArgs& args) {
		lockLayer(args.size() > 0 ? core::string::toInt(args[0]) : activeLayer(), true);
	}).setHelp("Lock a particular layer by id - or the current active one");

	core::Command::registerCommand("togglelayerlock", [&] (const core::CmdArgs& args) {
		const int layerId = args.size() > 0 ? core::string::toInt(args[0]) : activeLayer();
		lockLayer(layerId, !isLocked(layerId));
	}).setHelp("Toggle the lock state of a particular layer by id - or the current active one");

	core::Command::registerCommand("layerunlock", [&] (const core::CmdArgs& args) {
		lockLayer(args.size() > 0 ? core::string::toInt(args[0]) : activeLayer(), false);
	}).setHelp("Unlock a particular layer by id - or the current active one");

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
		if (args.size() != 2) {
			Log::info("Usage: layerstate <layerid> <true|false>");
			return;
		}
		const int layerId = core::string::toInt(args[0]);
		const bool newVisibleState = core::string::toBool(args[1]);
		hideLayer(layerId, !newVisibleState);
	}).setHelp("Change the visible state of a layer");

	core::Command::registerCommand("togglelayerstate", [&] (const core::CmdArgs& args) {
		const int layerId = args.size() > 0 ? core::string::toInt(args[0]) : activeLayer();
		hideLayer(layerId, isVisible(layerId));
	}).setHelp("Toggle the visible state of a layer");

	core::Command::registerCommand("layerhideall", [&] (const core::CmdArgs& args) {
		for (int idx = 0; idx < (int)_layers.size(); ++idx) {
			hideLayer(idx, true);
		}
	}).setHelp("Hide all layers");

	core::Command::registerCommand("layerlockall", [&] (const core::CmdArgs& args) {
		for (int idx = 0; idx < (int)_layers.size(); ++idx) {
			lockLayer(idx, true);
		}
	}).setHelp("Lock all layers");

	core::Command::registerCommand("layerunlockall", [&] (const core::CmdArgs& args) {
		for (int idx = 0; idx < (int)_layers.size(); ++idx) {
			lockLayer(idx, false);
		}
	}).setHelp("Unlock all layers");

	core::Command::registerCommand("layerhideothers", [&] (const core::CmdArgs& args) {
		for (int idx = 0; idx < (int)_layers.size(); ++idx) {
			if (idx == activeLayer()) {
				hideLayer(idx, false);
				continue;
			}
			hideLayer(idx, true);
		}
	}).setHelp("Hide all layers");

	core::Command::registerCommand("layerrename", [&] (const core::CmdArgs& args) {
		if (args.size() == 1) {
			const int layerId = activeLayer();
			rename(layerId, args[0]);
		} else if (args.size() == 2) {
			const int layerId = core::string::toInt(args[0]);
			rename(layerId, args[1]);
		} else {
			Log::info("Usage: layerrename [<layerid>] newname");
		}
	}).setHelp("Rename the current layer or the given layer id");

	core::Command::registerCommand("layershowall", [&] (const core::CmdArgs& args) {
		for (int idx = 0; idx < (int)_layers.size(); ++idx) {
			hideLayer(idx, false);
		}
	}).setHelp("Show all layers");

	core::Command::registerCommand("layerduplicate", [&] (const core::CmdArgs& args) {
		const int layerId = args.size() > 0 ? core::string::toInt(args[0]) : activeLayer();
		duplicate(layerId);
	}).setHelp("Duplicates the current layer or the given layer id");

	core::Command::registerCommand("layermoveup", [&] (const core::CmdArgs& args) {
		const int layerId = args.size() > 0 ? core::string::toInt(args[0]) : activeLayer();
		moveUp(layerId);
	}).setHelp("Move the current layer or the given layer id up");

	core::Command::registerCommand("layermovedown", [&] (const core::CmdArgs& args) {
		const int layerId = args.size() > 0 ? core::string::toInt(args[0]) : activeLayer();
		moveDown(layerId);
	}).setHelp("Move the current layer or the given layer id down");
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

bool LayerManager::rename(int layerId, const core::String& name) {
	if (!isValidLayerId(layerId)) {
		return false;
	}
	layer(layerId).name = name;
	for (auto& listener : _listeners) {
		listener->onLayerChanged(layerId);
	}
	return true;
}

bool LayerManager::duplicate(int layerId) {
	if (!isValidLayerId(layerId)) {
		return false;
	}
	// the only thing that we can do here is to inform the listeners about the wish to duplicate
	// we don't manage the volumes here
	const int n = validLayers();
	for (auto& listener : _listeners) {
		listener->onLayerDuplicate(layerId);
	}
	return validLayers() == n + 1;
}

bool LayerManager::hasValidLayerAfter(int layerId, int& id) const {
	for (int i = layerId + 1; i < (int)_layers.size(); ++i) {
		if (_layers[i].valid) {
			id = i;
			return true;
		}
	}
	id = -1;
	return false;
}

bool LayerManager::hasValidLayerBefore(int layerId, int& id) const {
	for (int i = layerId - 1; i >= 0; --i) {
		if (_layers[i].valid) {
			id = i;
			return true;
		}
	}
	id = -1;
	return false;
}

bool LayerManager::moveUp(int layerId) {
	int prevLayerId;
	if (!hasValidLayerBefore(layerId, prevLayerId) || layerId >= (int)_layers.size()) {
		Log::error("Failed to move layer %i up", layerId);
		return false;
	}
	Log::debug("move layer %i up", layerId);
	std::swap(_layers[layerId], _layers[prevLayerId]);
	for (auto& listener : _listeners) {
		listener->onLayerSwapped(layerId, prevLayerId);
	}
	setActiveLayer(prevLayerId);
	return true;
}

bool LayerManager::moveDown(int layerId) {
	int nextLayerId;
	if (layerId < 0 || !hasValidLayerAfter(layerId, nextLayerId)) {
		Log::error("Failed to move layer %i down", layerId);
		return false;
	}
	Log::debug("move layer %i down", layerId);
	std::swap(_layers[layerId], _layers[nextLayerId]);
	for (auto& listener : _listeners) {
		listener->onLayerSwapped(layerId, nextLayerId);
	}
	setActiveLayer(nextLayerId);
	return true;
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

bool LayerManager::isVisible(int layerId) const {
	if (layerId < 0 || layerId >= (int)_layers.size()) {
		Log::debug("Invalid layer id given: %i - can't answer visible-state request", layerId);
		return false;
	}
	if (!_layers[layerId].valid) {
		Log::debug("Attempt to request the visible-state for an invalid layer id: %i", layerId);
		return false;
	}
	return _layers[layerId].visible;
}

bool LayerManager::isLocked(int layerId) const {
	if (layerId < 0 || layerId >= (int)_layers.size()) {
		Log::debug("Invalid layer id given: %i - can't answer lock-state request", layerId);
		return false;
	}
	if (!_layers[layerId].valid) {
		Log::debug("Attempt to request the lock-state for an invalid layer id: %i", layerId);
		return false;
	}
	return _layers[layerId].locked;
}

void LayerManager::hideLayer(int layerId, bool hide) {
	if (layerId < 0 || layerId >= (int)_layers.size()) {
		Log::debug("Invalid layer id given: %i - can't perform visible-state-change", layerId);
		return;
	}
	if (!_layers[layerId].valid) {
		Log::debug("Attempt to change the visible-state for an invalid layer id: %i", layerId);
		return;
	}
	_layers[layerId].visible = !hide;
	for (auto& listener : _listeners) {
		if (hide) {
			listener->onLayerHide(layerId);
		} else {
			listener->onLayerShow(layerId);
		}
	}
}

void LayerManager::lockLayer(int layerId, bool lock) {
	if (layerId < 0 || layerId >= (int)_layers.size()) {
		Log::debug("Invalid layer id given: %i - can't perform lock", layerId);
		return;
	}
	if (!_layers[layerId].valid) {
		Log::debug("Attempt to lock an invalid layer id: %i", layerId);
		return;
	}
	_layers[layerId].locked = lock;
	for (auto& listener : _listeners) {
		if (lock) {
			listener->onLayerLocked(layerId);
		} else {
			listener->onLayerUnlocked(layerId);
		}
	}
}

void LayerManager::foreachGroupLayer(const std::function<void(int)>& f) {
	int layerId = activeLayer();
	if (layer(layerId).locked) {
		layerId = nextLockedLayer(-1);
		core_assert(layerId != -1);
		while (layerId != -1) {
			f(layerId);
			layerId = nextLockedLayer(layerId);
		}
	} else {
		f(layerId);
	}
}

int LayerManager::nextLockedLayer(int last) const {
	++last;
	if (last < 0) {
		return -1;
	}
	const int n = _layers.size();
	for (int i = last; i < n; ++i) {
		if (_layers[i].locked) {
			return i;
		}
	}
	return -1;
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
	if (!isValidLayerId(layerId)) {
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
	core_trace_scoped(DeleteLayer);
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

void LayerManager::addMetadata(int layerId, const LayerMetadata& metadata) {
	core_assert_always(layerId >= 0 && layerId < (int)_layers.size());
	for (const auto& m : metadata) {
		_layers[layerId].metadata.put(m->key, m->value);
	}
}

const LayerMetadata& LayerManager::metadata(int layerId) const {
	core_assert_always(layerId >= 0 && layerId < (int)_layers.size());
	return _layers[layerId].metadata;
}

int LayerManager::addLayer(const char *name, bool visible, voxel::RawVolume* volume, const glm::ivec3& pivot) {
	core_trace_scoped(AddLayer);
	const size_t maxLayers = _layers.size();
	for (size_t layerId = 0; layerId < maxLayers; ++layerId) {
		if (_layers[layerId].valid) {
			continue;
		}
		activateLayer(layerId, name, visible, volume, voxel::Region::InvalidRegion, pivot);
		return (int)layerId;
	}
	return -1;
}

bool LayerManager::activateLayer(int layerId, const char *name, bool visible, voxel::RawVolume* volume, const voxel::Region& region, const glm::ivec3& pivot) {
	core_trace_scoped(ActivateLayer);
	core_assert_always(layerId >= 0 && layerId < (int)_layers.size());
	if (name == nullptr || name[0] == '\0') {
		_layers[layerId].name = core::string::format("%i", (int)layerId);
	} else {
		_layers[layerId].name = name;
	}
	_layers[layerId].visible = visible;
	_layers[layerId].valid = volume != nullptr;
	_layers[layerId].pivot = pivot;
	for (auto& listener : _listeners) {
		listener->onLayerAdded(layerId, _layers[layerId], volume, region);
	}
	return true;
}

bool LayerManager::isValidLayerId(int layerId) const {
	if (layerId < 0 || layerId >= (int)_layers.size()) {
		Log::debug("Given layer %i is out of bounds", layerId);
		return false;
	}
	if (!_layers[layerId].valid) {
		Log::debug("Given layer %i is not valid", layerId);
		return false;
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
