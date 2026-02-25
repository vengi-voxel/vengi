/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "memento/MementoHandler.h"
#include "palette/NormalPalette.h"
#include "voxedit-util/network/ProtocolIds.h"

namespace voxedit {

/**
 * @brief Scene graph node normal palette changed message
 */
class NodeNormalPaletteChangedMessage : public network::ProtocolMessage {
private:
	core::UUID _nodeUUID;
	palette::NormalPalette _palette;

public:
	NodeNormalPaletteChangedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_NORMAL_PALETTE_CHANGED) {
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID in NodeNormalPaletteChangedMessage ctor");
			return;
		}
		if (!serializeNormalPalette(state.normalPalette)) {
			Log::error("Failed to serialize normal palette in NodeNormalPaletteChangedMessage ctor");
			return;
		}
		writeSize();
	}

	NodeNormalPaletteChangedMessage(network::MessageStream &in) {
		_id = PROTO_NODE_NORMAL_PALETTE_CHANGED;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for normal palette changed");
			return;
		}
		if (!deserializeNormalPalette(in, _palette)) {
			Log::error("Failed to deserialize normal palette for node %s", _nodeUUID.str().c_str());
			return;
		}
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in NodeNormalPaletteChangedMessage::writeBack");
			return;
		}
		if (!writeUUID(_nodeUUID)) {
			Log::error("Failed to write node UUID in NodeNormalPaletteChangedMessage::writeBack");
			return;
		}
		if (!serializeNormalPalette(_palette)) {
			Log::error("Failed to serialize normal palette in NodeNormalPaletteChangedMessage::writeBack");
			return;
		}
		writeSize();
	}

	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}

	const palette::NormalPalette &palette() const {
		return _palette;
	}
};

} // namespace voxedit
