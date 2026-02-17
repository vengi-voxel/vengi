/**
 * @file
 */

#pragma once

#include "memento/MementoHandler.h"
#include "voxedit-util/network/ProtocolIds.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"

namespace voxedit {

/**
 * @brief Voxel modification message with compressed voxel data
 */
class VoxelModificationMessage : public network::ProtocolMessage {
private:
	core::UUID _nodeUUID;
	voxel::Region _region;
	voxel::Region _volumeRegion;
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
			Log::error("Failed to serialize data region in VoxelModificationMessage ctor");
			return;
		}
		if (!serializeRegion(state.volumeRegion())) {
			Log::error("Failed to serialize volume region in VoxelModificationMessage ctor");
			return;
		}
		if (!serializeVolume(state.data.buffer(), state.data.size())) {
			Log::error("Failed to serialize volume in VoxelModificationMessage ctor");
			return;
		}
		writeSize();
	}
	/**
	 * @brief Construct a voxel modification message with direct parameters
	 * @param nodeUUID The UUID of the node to modify
	 * @param volume The volume containing the voxel data to send
	 * @param region The region within the volume to send (if invalid, the full volume region is used)
	 *
	 * @note The volume data will be compressed using MementoData::fromVolume
	 */
	VoxelModificationMessage(const core::UUID &nodeUUID, const voxel::RawVolume &volume,
							 const voxel::Region &region = voxel::Region::InvalidRegion)
		: ProtocolMessage(PROTO_VOXEL_MODIFICATION) {
		if (!writeUUID(nodeUUID)) {
			Log::error("Failed to write node UUID in VoxelModificationMessage ctor");
			return;
		}
		memento::MementoData data = memento::MementoData::fromVolume(&volume, region);
		if (!data.hasVolume()) {
			Log::error("Failed to compress volume data in VoxelModificationMessage ctor");
			return;
		}
		if (!serializeRegion(data.dataRegion())) {
			Log::error("Failed to serialize data region in VoxelModificationMessage ctor");
			return;
		}
		if (!serializeRegion(volume.region())) {
			Log::error("Failed to serialize volume region in VoxelModificationMessage ctor");
			return;
		}
		if (!serializeVolume(data.buffer(), data.size())) {
			Log::error("Failed to serialize volume in VoxelModificationMessage ctor");
			return;
		}
		writeSize();
	}
	VoxelModificationMessage(network::MessageStream &in) {
		_id = PROTO_VOXEL_MODIFICATION;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for voxel modification");
			return;
		}
		if (!deserializeRegion(in, _region)) {
			Log::error("Failed to deserialize data region for voxel modification");
			return;
		}
		if (!deserializeRegion(in, _volumeRegion)) {
			Log::error("Failed to deserialize volume region for voxel modification");
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
			Log::error("Failed to serialize data region in VoxelModificationMessage::writeBack");
			return;
		}
		if (!serializeRegion(_volumeRegion)) {
			Log::error("Failed to serialize volume region in VoxelModificationMessage::writeBack");
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
	const voxel::Region &volumeRegion() const {
		return _volumeRegion;
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


} // namespace voxedit
