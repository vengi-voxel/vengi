/**
 * @file
 */

#include "BenShared.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/RawVolume.h"

namespace voxelformat {
namespace benv {

ScopedChunkCheck::ScopedChunkCheck(io::SeekableReadStream &stream) : _stream(stream) {
	if (_stream.readUInt32(id) != 0) {
		Log::error("Failed to read chunk id");
		return;
	}
	if (_stream.readUInt32(length) != 0) {
		Log::error("Failed to read length of riff header");
		return;
	}
	_chunkPos = stream.pos();
	uint8_t buf[4];
	FourCCRev(buf, id);
	Log::debug("benv chunk of size %i: %c%c%c%c", (int)length, buf[0], buf[1], buf[2], buf[3]);
}

ScopedChunkCheck::~ScopedChunkCheck() {
	int64_t expectedPos = _chunkPos + length;
	if (_stream.pos() != expectedPos) {
		uint8_t buf[4];
		FourCCRev(buf, id);
		Log::warn("benv chunk has unexpected size of %i - expected was %i: %c%c%c%c", (int)(_stream.pos() - _chunkPos),
				  (int)length, buf[0], buf[1], buf[2], buf[3]);
		_stream.seek(expectedPos);
	}
}

bool addPointNode(scenegraph::SceneGraph &sceneGraph, const core::String &name, const glm::vec3 &pointPos, int parent) {
	scenegraph::SceneGraphNode pointNode(scenegraph::SceneGraphNodeType::Point);
	pointNode.setName(name);
	scenegraph::SceneGraphTransform transform;
	transform.setLocalTranslation(pointPos);
	pointNode.setTransform(0, transform);
	return sceneGraph.emplace(core::move(pointNode), parent) != InvalidNodeId;
}

// static bool loadLeaf(io::SeekableReadStream &stream, scenegraph::SceneGraphNode &node, const voxel::Voxel &voxel) {
// 	return false;
// }

static bool loadBranch(io::SeekableReadStream &stream, scenegraph::SceneGraphNode &node, int depth) {
	uint8_t header;
	if (stream.readUInt8(header) != 0) {
		Log::error("Failed to read header");
		return InvalidNodeId;
	}
	// const uint8_t octant = header & 7;
	const uint8_t type = (header >> 6) & 3;
	switch (type) {
	case 0: { // regular branch
		const uint8_t children = (header >> 3 & 7) + 1;
		for (uint8_t i = 0; i < children; ++i) {
			uint8_t childHeader;
			if (stream.peekUInt8(childHeader) != 0) {
				Log::error("Failed to read child header");
				return InvalidNodeId;
			}
			// const uint8_t childOctant = childHeader & 7;
			if ((childHeader & 0b10000000) > 0) {
				// TODO: VOXELFORMAT: load the Sparse Voxel Octree
				// if (!loadLeaf(stream, node)) {
				// 	return false;
				// }
			} else {
				if (!loadBranch(stream, node, depth++)) {
					return false;
				}
			}
		}
		break;
	}
	case 1: { // collapsed branch
		uint8_t color;
		if (stream.readUInt8(color) != 0) {
			Log::error("Failed to read color for collapsed branch");
			return InvalidNodeId;
		}
		// TODO: VOXELFORMAT: load the Sparse Voxel Octree
		// voxel::Voxel voxel = color == 0 ? voxel::Voxel() : voxel::createVoxel(node.palette(), color);
		if (depth == 15) {
			for (uint8_t c_octant = 0u; c_octant < 8u; c_octant++) {
				// loadLeafVoxels(stream, node, c_octant, voxel);
			}
		} else {
			for (uint8_t c_octant = 0u; c_octant < 8u; c_octant++) {
				loadBranch(stream, node, /*voxel, */ depth);
			}
		}
		break;
	}
	}
	return true;
}

int createModelNode(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const core::String &name,
					int width, int height, int depth, io::SeekableReadStream &stream, const Metadata &globalMetadata,
					const Metadata &metadata) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(name);
	voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	node.setPalette(palette);
	node.setVolume(v, true);

	int svoDepth = 0;
	while (!stream.eos()) {
		if (!loadBranch(stream, node, svoDepth)) {
			Log::error("Failed to load branch");
			return InvalidNodeId;
		}
	}

	int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to create model node");
	}
	return nodeId;
}

} // namespace benv
} // namespace voxelformat
