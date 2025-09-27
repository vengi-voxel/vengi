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
		writeUUID(state.nodeUUID);
		core_assert_always(state.dataRegion().isValid());
		serializeRegion(state.dataRegion());
		serializeVolume(state.data.buffer(), state.data.size());
		writeSize();
	}
	VoxelModificationMessage(MessageStream &in) {
		_id = PROTO_VOXEL_MODIFICATION;
		in.readUUID(_nodeUUID);
		deserializeRegion(in, _region);
		deserializeVolume(in, _compressedSize, _compressedData);
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writeUUID(_nodeUUID);
		serializeRegion(_region);
		serializeVolume(_compressedData, _compressedSize);
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
