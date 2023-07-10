/**
 * @file
 */

#pragma once

#include "voxelformat/private/mesh/MeshFormat.h"
#include "core/collection/StringMap.h"
#include "image/Image.h"

namespace voxelformat {
/**
 * @brief Quake2 model format
 *
 * @ingroup Formats
 */
class MD2Format : public MeshFormat {
public:
	struct MD2Header {
		uint32_t magic;
		uint32_t version;
		uint32_t skinWidth;
		uint32_t skinHeight;
		uint32_t frameSize;
		uint32_t numSkins;
		uint32_t numVerts;
		uint32_t numST;
		uint32_t numTris;
		uint32_t numGLCmds;
		uint32_t numFrames;
		uint32_t offsetSkins;
		uint32_t offsetST;
		uint32_t offsetTris;
		uint32_t offsetFrames;
		uint32_t offsetGLCmds;
		uint32_t offsetEnd;
	};
	static_assert(sizeof(MD2Header) == 68, "MD2Header size is wrong");

	struct MD2Vertex {
		glm::bvec4 vertex; // scaled byte to fit frame mins/maxs
						   // only x, y and z are used, w is a padding byte
	};
	static_assert(sizeof(MD2Vertex) == 4, "MD2Vertex size is wrong");

	struct MD2FrameHeader {
		glm::vec3 scale;
		glm::vec3 translate;
		char name[16];
		// after this numVerts * MD2Vertex data follows
	};
	static_assert(sizeof(MD2FrameHeader) == 40, "MD2FrameHeader size is wrong");

private:
	bool voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;
	bool loadFrame(const core::String &filename, io::SeekableReadStream &stream, int64_t startOffset,
				   const MD2Header &hdr, uint32_t frameIndex, scenegraph::SceneGraph &sceneGraph,
				   const core::StringMap<image::ImagePtr> &textures);

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
					const glm::vec3 &scale = glm::vec3(1.0f), bool quad = false, bool withColor = true,
					bool withTexCoords = true) override {
		return false;
	}
};
} // namespace voxelformat
