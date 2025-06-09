/**
 * @file
 */

#pragma once

#include "core/collection/StringMap.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelformat {

namespace benv {

class ScopedChunkCheck {
private:
	io::SeekableReadStream &_stream;
	int64_t _chunkPos;
	bool _check;

public:
	uint32_t id;
	uint32_t length;
	ScopedChunkCheck(io::SeekableReadStream &stream, bool check = true);
	~ScopedChunkCheck();
};

class Chunk {
private:
	uint32_t _id;
	io::SeekableWriteStream &_stream;
	int64_t _lengthPos = 0;

public:
	Chunk(io::SeekableWriteStream &stream, uint32_t id);
	~Chunk();
};

struct PointNode {
	PointNode(const core::String &n, const glm::vec3 &pos) : name(n), pointPos(pos) {
	}
	core::String name;
	glm::vec3 pointPos;
};

struct Metadata {
	core::StringMap<palette::Palette> palettes;
	scenegraph::SceneGraphNodeProperties properties;
	core::DynamicArray<PointNode> points; // coordinates in vengi coordinate system
};

bool addPointNode(scenegraph::SceneGraph &sceneGraph, const core::String &name, const glm::vec3 &pointPos,
				  int parent = -1);

int createModelNode(scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const core::String &name, int width,
					int height, int depth, io::SeekableReadStream &stream, const Metadata &globalMetadata,
					const Metadata &metadata);

bool saveModel(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
			   io::WriteStream &stream, bool includeSizes);
Metadata createMetadata(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node);

} // namespace benv
} // namespace voxelformat
