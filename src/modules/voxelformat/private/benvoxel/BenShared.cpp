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
#include "voxelformat/private/benvoxel/SparseVoxelOctree.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {
namespace benv {

ScopedChunkCheck::ScopedChunkCheck(io::SeekableReadStream &stream, bool check) : _stream(stream), _check(check) {
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
	Log::debug("benv chunk of size %i (remaining %i): %c%c%c%c", (int)length, (int)stream.remaining(), buf[0], buf[1],
			   buf[2], buf[3]);
}

ScopedChunkCheck::~ScopedChunkCheck() {
	if (!_check) {
		return;
	}
	int64_t expectedPos = _chunkPos + length;
	if (_stream.pos() != expectedPos) {
		uint8_t buf[4];
		FourCCRev(buf, id);
		Log::warn("benv chunk has unexpected size of %i - expected was %i: %c%c%c%c", (int)(_stream.pos() - _chunkPos),
				  (int)length, buf[0], buf[1], buf[2], buf[3]);
		_stream.seek(expectedPos);
	}
}

Chunk::Chunk(io::SeekableWriteStream &stream, uint32_t id) : _id(id), _stream(stream) {
	if (!stream.writeUInt32(_id)) {
		Log::error("Failed to write chunk id");
		return;
	}
	_lengthPos = stream.pos();
	if (!stream.writeUInt32(0)) {
		Log::error("Failed to write length of riff header");
		return;
	}
	uint8_t buf[4];
	FourCCRev(buf, _id);
	Log::debug("save benv chunk: %c%c%c%c", buf[0], buf[1], buf[2], buf[3]);
}

Chunk::~Chunk() {
	int64_t endPos = _stream.pos();
	_stream.seek(_lengthPos);
	const uint32_t length = (uint32_t)(endPos - _lengthPos - 4);
	if (!_stream.writeUInt32(length)) {
		Log::error("Failed to write length of riff header");
	}
	uint8_t buf[4];
	FourCCRev(buf, _id);
	Log::debug("saved benv chunk of size %i: %c%c%c%c", (int)length, buf[0], buf[1], buf[2], buf[3]);
	_stream.seek(endPos);
}

bool addPointNode(scenegraph::SceneGraph &sceneGraph, const core::String &name, const glm::vec3 &pointPos, int parent) {
	scenegraph::SceneGraphNode pointNode(scenegraph::SceneGraphNodeType::Point);
	pointNode.setName(name);
	scenegraph::SceneGraphTransform transform;
	transform.setLocalTranslation(pointPos);
	pointNode.setTransform(0, transform);
	return sceneGraph.emplace(core::move(pointNode), parent) != InvalidNodeId;
}

Metadata createMetadata(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node) {
	Metadata metadata;
	for (int child : node.children()) {
		const scenegraph::SceneGraphNode &cnode = sceneGraph.node(child);
		if (cnode.isPointNode()) {
			const core::String &name = node.name();
			const glm::vec3 &pointPos = node.transform(0).localTranslation();
			metadata.points.emplace_back(name, glm::ivec3{pointPos.x, pointPos.y, pointPos.z});
		}
	}

	// point nodes are used to model negative space
	if (node.isModelNode()) {
		const voxel::Region &region = sceneGraph.resolveRegion(node);
		if (region.getLowerCorner() != glm::ivec3(0)) {
			// empty name is for modelling a region offset
			metadata.points.emplace_back("", region.getLowerCorner());
		}
	}

	metadata.properties = node.properties();
	// default palette has empty name
	if (node.hasPalette()) {
		metadata.palettes.put("", node.palette());
	}
	return metadata;
}

bool saveModel(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
			   io::WriteStream &stream, bool includeSizes) {
	const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
	if (!volume) {
		Log::error("No volume found for model node %i", node.id());
		return false;
	}

	const voxel::Region &region = volume->region();
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	Log::debug("Saving model with size: %d:%d:%d", dim.x, dim.y, dim.z);

	BenVoxel::SparseVoxelOctree svo(dim.x, dim.z, dim.y);
	voxelutil::visitVolume(*volume, [&svo, dim](int x, int y, int z, const voxel::Voxel &voxel) {
		BenVoxel::SVOVoxel svoVoxel(dim.x - 1 - x, z, y, voxel.getColor() + 1);
		svo.set(svoVoxel);
	});

	svo.write(stream, includeSizes);
	return true;
}

int createModelNode(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const core::String &name, int width,
					int height, int depth, io::SeekableReadStream &stream, const Metadata &globalMetadata,
					const Metadata &metadata) {
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(name);
	voxel::Region region(0, 0, 0, width - 1, height - 1, depth - 1);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	if (!metadata.palettes.get("", palette)) {
		globalMetadata.palettes.get("", palette);
	}
	node.setPalette(palette);
	node.setVolume(v, true);

	BenVoxel::SparseVoxelOctree svo(stream, (uint16_t)width, (uint16_t)depth, (uint16_t)height);
	Log::debug("Found %i voxels in volume with size: %d:%d:%d", (int)svo.voxels().size(), width, height, depth);
	for (const BenVoxel::SVOVoxel &voxel : svo.voxels()) {
		v->setVoxel(width - 1 - (int32_t)voxel.position.x, (int32_t)voxel.position.z, (int32_t)voxel.position.y,
					voxel::createVoxel(voxel::VoxelType::Generic, voxel.index));
	}

	int nodeId = sceneGraph.emplace(core::move(node));
	if (nodeId == InvalidNodeId) {
		Log::error("Failed to create model node");
	}
	return nodeId;
}

} // namespace benv
} // namespace voxelformat
