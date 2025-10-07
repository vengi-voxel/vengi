/**
 * @file
 */

#pragma once

#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
#include "io/StreamArchive.h"
#include "scenegraph/SceneGraph.h"
#include "util/VarUtil.h"
#include "voxedit-util/network/ProtocolMessage.h"
#include "voxelformat/private/vengi/VENGIFormat.h"

namespace voxedit {
namespace network {

class SceneStateMessage : public ProtocolMessage {
private:
	scenegraph::SceneGraph _sceneGraph;

public:
	SceneStateMessage(scenegraph::SceneGraph &sceneGraph) : ProtocolMessage(PROTO_SCENE_STATE) {
		// we don't want to modify the voxels
		util::ScopedVarChange var1(cfg::VoxformatEmptyPaletteIndex, "-1");

		voxelformat::VENGIFormat vengiFormat;
		io::SeekableWriteStream *writeStream = (io::SeekableWriteStream *)this;
		const io::StreamArchivePtr &archive = io::openStreamArchive(writeStream);
		voxelformat::SaveContext ctx;
		vengiFormat.saveGroups(sceneGraph, "net.vengi", archive, ctx);
		writeSize();
	}

	SceneStateMessage(MessageStream &in, uint32_t size) {
		_id = PROTO_SCENE_STATE;
		// we don't want to modify the voxels
		util::ScopedVarChange var1(cfg::VoxformatEmptyPaletteIndex, "-1");

		voxelformat::VENGIFormat vengiFormat;
		io::BufferedReadWriteStream bufferedStream(in, size);
		io::SeekableReadStream *readStream = (io::SeekableReadStream *)&bufferedStream;
		const io::StreamArchivePtr &archive = io::openStreamArchive(readStream);
		voxelformat::LoadContext ctx;
		vengiFormat.load("net.vengi", archive, _sceneGraph, ctx);
	}
	// this is intentionally not complete - as this message is not broadcasted because it is sent by the server and
	// never coming from a client
	void writeBack() override {
		if (!writeInt32(0) || !writeUInt8(_id)) {
			Log::error("Failed to write header in SceneStateMessage::writeBack");
			return;
		}
		writeSize();
	}

	scenegraph::SceneGraph &sceneGraph() {
		return _sceneGraph;
	}
};

} // namespace network
} // namespace voxedit
