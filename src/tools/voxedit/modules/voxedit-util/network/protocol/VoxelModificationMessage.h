/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "memento/MementoHandler.h"
#include "voxedit-util/network/ProtocolMessage.h"
#include "voxel/Region.h"

namespace voxedit {
namespace network {

/**
 * @brief Voxel modification message with compressed voxel data
 */
class VoxelModificationMessage : public ProtocolMessage {
private:
	core::UUID _nodeUUID;
	voxel::Region _region;
	uint32_t _compressedSize = 0;
	uint8_t *_compressedData = nullptr;

public:
	VoxelModificationMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_VOXEL_MODIFICATION) {
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID in VoxelModificationMessage ctor");
			return;
		}
		core_assert_always(state.dataRegion().isValid());
		if (!serializeRegion(state.dataRegion())) {
			Log::error("Failed to serialize region in VoxelModificationMessage ctor");
			return;
		}
		if (!serializeVolume(state.data.buffer(), state.data.size())) {
			Log::error("Failed to serialize volume in VoxelModificationMessage ctor");
			return;
		}
		writeSize();
	}
	VoxelModificationMessage(MessageStream &in) {
		_id = PROTO_VOXEL_MODIFICATION;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for voxel modification");
			return;
		}
		if (!deserializeRegion(in, _region)) {
			Log::error("Failed to deserialize region for voxel modification");
			return;
		}
		if (!deserializeVolume(in, _compressedSize, _compressedData)) {
			Log::error("Failed to deserialize volume for voxel modification");
			return;
		}
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in VoxelModificationMessage::writeBack");
			return;
		}
		if (!writeUUID(_nodeUUID)) {
			Log::error("Failed to write node UUID in VoxelModificationMessage::writeBack");
			return;
		}
		if (!serializeRegion(_region)) {
			Log::error("Failed to serialize region in VoxelModificationMessage::writeBack");
			return;
		}
		if (!serializeVolume(_compressedData, _compressedSize)) {
			Log::error("Failed to serialize volume in VoxelModificationMessage::writeBack");
			return;
		}
		writeSize();
	}

	~VoxelModificationMessage() override {
		delete[] _compressedData;
	}

	const voxel::Region &region() const {
		return _region;
	}
	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
	const uint8_t *compressedData() const {
		return _compressedData;
	}
	uint32_t compressedSize() const {
		return _compressedSize;
	}
};

} // namespace network
} // namespace voxedit
