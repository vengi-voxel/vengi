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

int createModelNode(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const core::String &name,
					int width, int height, int depth, io::SeekableReadStream &stream, const Metadata &globalMetadata,
					const Metadata &metadata) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(name);
	voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	node.setPalette(palette);
	node.setVolume(v, true);

	io::SeekableReadStreamAdapter adapter(stream);
	BenVoxel::SparseVoxelOctree svo(adapter, static_cast<uint16_t>(width), static_cast<uint16_t>(depth),
									static_cast<uint16_t>(height));
	for (BenVoxel::Voxel voxel : svo.voxels())
		v->setVoxel(static_cast<int32_t>(voxel.x), static_cast<int32_t>(voxel.z), static_cast<int32_t>(voxel.y),
					voxel::createVoxel(voxel::VoxelType::Generic, voxel.index, 255, 0));

	int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to create model node");
	}
	return nodeId;
}

} // namespace benv
} // namespace voxelformat
