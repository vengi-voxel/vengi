/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "memento/MementoHandler.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "voxedit-util/network/ProtocolMessage.h"

namespace voxedit {
namespace network {

/**
 * @brief Scene graph node palette changed message
 */
class NodePaletteChangedMessage : public ProtocolMessage {
private:
	core::UUID _nodeUUID;
	palette::Palette _palette;

public:
	NodePaletteChangedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_PALETTE_CHANGED) {
		if (!writeUUID(state.nodeUUID)) {
			Log::error("Failed to write node UUID in NodePaletteChangedMessage ctor");
			return;
		}
		if (!serializePalette(state.palette)) {
			Log::error("Failed to serialize palette in NodePaletteChangedMessage ctor");
			return;
		}
		writeSize();
	}

	NodePaletteChangedMessage(MessageStream &in) {
		_id = PROTO_NODE_PALETTE_CHANGED;
		if (in.readUUID(_nodeUUID) == -1) {
			Log::error("Failed to read node UUID for palette changed");
			return;
		}
		if (!deserializePalette(in, _palette)) {
			Log::error("Failed to deserialize palette for node %s", _nodeUUID.str().c_str());
			return;
		}
	}
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in NodePaletteChangedMessage::writeBack");
			return;
		}
		if (!writeUUID(_nodeUUID)) {
			Log::error("Failed to write node UUID in NodePaletteChangedMessage::writeBack");
			return;
		}
		if (!serializePalette(_palette)) {
			Log::error("Failed to serialize palette in NodePaletteChangedMessage::writeBack");
			return;
		}
		writeSize();
	}

	const core::UUID &nodeUUID() const {
		return _nodeUUID;
	}
	const palette::Palette &palette() const {
		return _palette;
	}
};

} // namespace network
} // namespace voxedit
