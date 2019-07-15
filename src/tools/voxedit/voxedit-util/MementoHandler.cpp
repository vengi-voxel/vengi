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

static const MementoState InvalidLayerState{MementoType::Modification, MementoData(), -1, "", voxel::Region::InvalidRegion};
const int MementoHandler::MaxStates = 64;

MementoData::MementoData(const uint8_t* buf, size_t bufSize,
		const voxel::Region& _region) :
		compressedSize(bufSize), region(_region) {
	if (buf != nullptr) {
		core_assert(compressedSize > 0);
		buffer = new uint8_t[compressedSize];
		memcpy(buffer, buf, compressedSize);
	} else {
		core_assert(compressedSize == 0);
	}
}

MementoData::MementoData(MementoData&& o) :
		compressedSize(std::exchange(o.compressedSize, 0)),
		buffer(std::exchange(o.buffer, nullptr)),
		region(o.region) {
}

MementoData::~MementoData() {
	if (buffer != nullptr) {
		delete[] buffer;
		buffer = nullptr;
	}
}

MementoData::MementoData(const MementoData& o) :
		compressedSize(o.compressedSize),
		region(o.region) {
	if (o.buffer != nullptr) {
		core_assert(compressedSize > 0);
		buffer = new uint8_t[compressedSize];
		memcpy(buffer, o.buffer, compressedSize);
	} else {
		core_assert(compressedSize == 0);
	}
}

MementoData& MementoData::operator=(MementoData &&o) {
	if (this != &o) {
		compressedSize = std::exchange(o.compressedSize, 0);
		buffer = std::exchange(o.buffer, nullptr);
		region = o.region;
	}
	return *this;
}

MementoData MementoData::fromVolume(const voxel::RawVolume* volume) {
	if (volume == nullptr) {
		return MementoData();
	}
	const size_t uncompressedBufferSize = volume->region().voxels() * sizeof(voxel::Voxel);
	const uint32_t compressedBufferSize = core::zip::compressBound(uncompressedBufferSize);
	uint8_t *compressedBuf = new uint8_t[compressedBufferSize];
	size_t finalBufSize = 0u;
	if (!core::zip::compress(volume->data(), uncompressedBufferSize, compressedBuf, compressedBufferSize, &finalBufSize)) {
		delete[] compressedBuf;
		return MementoData();
	}
	const MementoData data(compressedBuf, finalBufSize, volume->region());
	delete[] compressedBuf;

	Log::debug("Memento state. Volume: %i, compressed: %i",
			(int)uncompressedBufferSize, (int)data.compressedSize);
	return data;
}

voxel::RawVolume* MementoData::toVolume(const MementoData& mementoData) {
	if (mementoData.buffer == nullptr) {
		return nullptr;
	}
	const size_t uncompressedBufferSize = mementoData.region.voxels() * sizeof(voxel::Voxel);
	uint8_t *uncompressedBuf = new uint8_t[uncompressedBufferSize];
	core::zip::uncompress(mementoData.buffer, mementoData.compressedSize, uncompressedBuf, uncompressedBufferSize);
	return voxel::RawVolume::createRaw((voxel::Voxel*)uncompressedBuf, mementoData.region);
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
		for (MementoState& state : _states) {
			const glm::ivec3& mins = state.region.getLowerCorner();
			const glm::ivec3& maxs = state.region.getUpperCorner();
			Log::info("%4i: %i - %s (%s) [mins(%i:%i:%i)/maxs(%i:%i:%i)]",
					i++, state.layer, state.name.c_str(), state.data.buffer == nullptr ? "empty" : "volume",
							mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		}
	});
}

void MementoHandler::clearStates() {
	_states.clear();
	_statePosition = 0u;
}

MementoState MementoHandler::undo() {
	if (!canUndo()) {
		return InvalidLayerState;
	}
	core_assert(_statePosition >= 1);
	--_statePosition;
	if (_states[_statePosition].data.buffer != nullptr
			&& _states[_statePosition].type == MementoType::LayerAdded
			&& _states[_statePosition + 1].type != MementoType::Modification) {
		--_statePosition;
	}
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	const MementoState& s = state();
	const voxel::Region region = _states[_statePosition + 1].region;
	voxel::logRegion("Undo", region);
	return MementoState{_states[_statePosition + 1].type, s.data, s.layer, s.name, region};
}

MementoState MementoHandler::redo() {
	if (!canRedo()) {
		return InvalidLayerState;
	}
	Log::debug("Available states: %i, current index: %i", (int)_states.size(), _statePosition);
	++_statePosition;
	if (_states[_statePosition].data.buffer == nullptr && _states[_statePosition].type == MementoType::LayerAdded) {
		++_statePosition;
	}
	if (_states[_statePosition].data.buffer != nullptr && _states[_statePosition].type == MementoType::LayerDeleted) {
		++_statePosition;
	}
	const MementoState& s = state();
	voxel::logRegion("Redo", s.region);
	return MementoState{s.type, s.data, s.layer, s.name, s.region};
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
		_states.erase(iStates, _states.end());
	}
	Log::debug("New undo state for layer %i with name %s (memento state index: %i)", layer, name.c_str(), (int)_states.size());
	voxel::logRegion("MarkUndo", region);
	const MementoData& data = MementoData::fromVolume(volume);
	_states.push_back(MementoState{type, data, layer, name, region});
	while (_states.size() > MaxStates) {
		_states.erase(_states.begin());
	}
	_statePosition = stateSize() - 1;
}

}
