/**
 * @file
 */

#include "MementoHandler.h"

#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/Region.h"
#include "core/command/Command.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/Zip.h"

namespace voxedit {

static const LayerState InvalidLayerState{MementoType::Modification, nullptr, -1, "", voxel::Region::InvalidRegion};
const int MementoHandler::MaxStates = 64;

LayerVolumeData* LayerVolumeData::fromVolume(const voxel::RawVolume* volume) {
	if (volume == nullptr) {
		return nullptr;
	}
	LayerVolumeData* data = new LayerVolumeData();
	data->region = volume->region();
	const size_t uncompressedBufferSize = data->region.voxels() * sizeof(voxel::Voxel);
	const uint32_t compressedBufferSize = core::zip::compressBound(uncompressedBufferSize);
	uint8_t *compressedBuf = new uint8_t[compressedBufferSize];
	data->compressedBufferSize = 0;
	if (!core::zip::compress(volume->data(), uncompressedBufferSize, compressedBuf, compressedBufferSize, &data->compressedBufferSize)) {
		delete[] compressedBuf;
		return nullptr;
	}

	data->data = new uint8_t[data->compressedBufferSize];
	memcpy(data->data, compressedBuf, data->compressedBufferSize);
	delete[] compressedBuf;

	Log::debug("Memento state. Volume: %i, compressed: %i",
			(int)uncompressedBufferSize, (int)data->compressedBufferSize);
	return data;
}

voxel::RawVolume* LayerVolumeData::toVolume(const LayerVolumeData* data) {
	if (data == nullptr) {
		return nullptr;
	}
	const size_t uncompressedBufferSize = data->region.voxels() * sizeof(voxel::Voxel);
	uint8_t *uncompressedBuf = new uint8_t[uncompressedBufferSize];
	core::zip::uncompress(data->data, data->compressedBufferSize, uncompressedBuf, uncompressedBufferSize);
	return voxel::RawVolume::createRaw((voxel::Voxel*)uncompressedBuf, data->region);
}

MementoHandler::MementoHandler() {
}

MementoHandler::~MementoHandler() {
	shutdown();
}

bool MementoHandler::init() {
	_states.reserve(MaxStates);
	return true;
}

void MementoHandler::shutdown() {
	clearStates();
}

void MementoHandler::lock() {
	++_locked;
}

void MementoHandler::unlock() {
	--_locked;
}

void MementoHandler::construct() {
	core::Command::registerCommand("ve_mementoinfo", [&] (const core::CmdArgs& args) {
		Log::info("Current memento state index: %i", _statePosition);
		Log::info("Maximum memento states: %i", MaxStates);
		int i = 0;
		for (LayerState& state : _states) {
			const glm::ivec3& mins = state.region.getLowerCorner();
			const glm::ivec3& maxs = state.region.getUpperCorner();
			Log::info("%4i: %i - %s (%s) [mins(%i:%i:%i)/maxs(%i:%i:%i)]",
					i++, state.layer, state.name.c_str(), state.volume == nullptr ? "empty" : "volume",
							mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		}
	});
}

void MementoHandler::clearStates() {
	for (LayerState& state : _states) {
		delete state.volume;
	}
	_states.clear();
	_statePosition = 0u;
}

LayerState MementoHandler::undo() {
	if (!canUndo()) {
		return InvalidLayerState;
	}
	core_assert(_statePosition >= 1);
	--_statePosition;
	if (_states[_statePosition].volume != nullptr
			&& _states[_statePosition].type == MementoType::LayerAdded
			&& _states[_statePosition + 1].type != MementoType::Modification) {
		--_statePosition;
	}
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	const LayerState& s = state();
	const voxel::Region region = _states[_statePosition + 1].region;
	voxel::logRegion("Undo", region);
	return LayerState{_states[_statePosition + 1].type, s.volume, s.layer, s.name, region};
}

LayerState MementoHandler::redo() {
	if (!canRedo()) {
		return InvalidLayerState;
	}
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	++_statePosition;
	if (_states[_statePosition].volume == nullptr && _states[_statePosition].type == MementoType::LayerAdded) {
		++_statePosition;
	}
	if (_states[_statePosition].volume != nullptr && _states[_statePosition].type == MementoType::LayerDeleted) {
		++_statePosition;
	}
	const LayerState& s = state();
	voxel::logRegion("Redo", s.region);
	return LayerState{s.type, s.volume, s.layer, s.name, s.region};
}

void MementoHandler::markLayerDeleted(int layer, const std::string& name, const voxel::RawVolume* volume) {
	Log::debug("Mark layer %i as deleted (%s)", layer, name.c_str());
	// previous state is that we have a volume at the given layer
	markUndo(layer, name, volume, MementoType::LayerDeleted);
	// current state is that there is no volume at the given layer
	markUndo(layer, name, nullptr, MementoType::LayerDeleted);
}

void MementoHandler::markLayerAdded(int layer, const std::string& name, const voxel::RawVolume* volume) {
	Log::debug("Mark layer %i as added (%s)", layer, name.c_str());
	// previous state is that there is no volume at the given layer
	markUndo(layer, name, nullptr, MementoType::LayerAdded);
	// current state is that we have a volume at the given layer
	markUndo(layer, name, volume, MementoType::LayerAdded);
}

void MementoHandler::markUndo(int layer, const std::string& name, const voxel::RawVolume* volume, MementoType type, const voxel::Region& region) {
	if (_locked > 0) {
		return;
	}
	if (!_states.empty()) {
		// if we mark something as new undo state, we can throw away
		// every other state that follows the new one (everything after
		// the current state position)
		auto iStates = _states.begin();
		std::advance(iStates, _statePosition + 1);
		for (auto iter = iStates; iter < _states.end(); ++iter) {
			delete iter->volume;
		}
		_states.erase(iStates, _states.end());
	}
	Log::debug("New undo state for layer %i with name %s (memento state index: %i)", layer, name.c_str(), (int)_states.size());
	voxel::logRegion("MarkUndo", region);
	LayerVolumeData *data = LayerVolumeData::fromVolume(volume);
	_states.push_back(LayerState{type, data, layer, name, region});
	while (_states.size() > MaxStates) {
		delete _states.begin()->volume;
		_states.erase(_states.begin());
	}
	_statePosition = stateSize() - 1;
}

}
