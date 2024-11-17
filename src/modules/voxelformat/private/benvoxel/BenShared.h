/**
 * @file
 */

#pragma once

#include "core/collection/StringMap.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

namespace benv {

class ScopedChunkCheck {
private:
	io::SeekableReadStream &_stream;
	int64_t _chunkPos;

public:
	uint32_t id;
	uint32_t length;
	ScopedChunkCheck(io::SeekableReadStream &stream);
	~ScopedChunkCheck();
};

struct PointNode {
	PointNode(const core::String &n, const glm::vec3 &pos) : name(n), pointPos(pos) {
	}
	core::String name;
	glm::vec3 pointPos;
};

struct Metadata {
	core::StringMap<palette::Palette> palettes;
	core::StringMap<core::String> properties;
	core::DynamicArray<PointNode> points;
};

bool addPointNode(scenegraph::SceneGraph &sceneGraph, const core::String &name, const glm::vec3 &pointPos,
				  int parent = -1);

int createModelNode(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const core::String &name,
					int width, int height, int depth, io::SeekableReadStream &stream, const Metadata &globalMetadata,
					const Metadata &metadata);

} // namespace benv
} // namespace voxelformat
