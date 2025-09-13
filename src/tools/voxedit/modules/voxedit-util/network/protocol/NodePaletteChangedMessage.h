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
	core::String _nodeUUID;
	palette::Palette _palette;

public:
	NodePaletteChangedMessage(const memento::MementoState &state) : ProtocolMessage(PROTO_NODE_PALETTE_CHANGED) {
		writePascalStringUInt16LE(state.nodeUUID);
		serializePalette(state.palette);
		writeSize();
	}

	NodePaletteChangedMessage(MessageStream &in) {
		_id = PROTO_NODE_PALETTE_CHANGED;
		in.readPascalStringUInt16LE(_nodeUUID);
		deserializePalette(in, _palette);
	}
	void writeBack() override {
		writeInt32(0);
		writeUInt8(_id);
		writePascalStringUInt16LE(_nodeUUID);
		serializePalette(_palette);
		writeSize();
	}

	const core::String &nodeUUID() const {
		return _nodeUUID;
	}
	const palette::Palette &palette() const {
		return _palette;
	}
};

} // namespace network
} // namespace voxedit
