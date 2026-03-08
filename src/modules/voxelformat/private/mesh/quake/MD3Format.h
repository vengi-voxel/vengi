/**
 * @file
 */

#pragma once

#include "voxelformat/private/mesh/MeshFormat.h"

namespace voxelformat {
/**
 * @brief Quake 3 model format
 *
 * https://icculus.org/~phaethon/q3a/formats/md3format.html
 *
 * @ingroup Formats
 */
class MD3Format : public MeshFormat {
public:
	static constexpr int MD3_MAX_QPATH = 64;
	static constexpr float MD3_XYZ_SCALE = 1.0f / 64.0f;

	struct MD3Header {
		uint32_t magic;
		int32_t version;
		char name[MD3_MAX_QPATH];
		int32_t flags;
		int32_t numFrames;
		int32_t numTags;
		int32_t numSurfaces;
		int32_t numSkins;
		int32_t ofsFrames;
		int32_t ofsTags;
		int32_t ofsSurfaces;
		int32_t ofsEnd;
	};
	static_assert(sizeof(MD3Header) == 108, "MD3Header size is wrong");

	struct MD3Frame {
		float minBounds[3];
		float maxBounds[3];
		float localOrigin[3];
		float radius;
		char name[16];
	};
	static_assert(sizeof(MD3Frame) == 56, "MD3Frame size is wrong");

	struct MD3Tag {
		char name[MD3_MAX_QPATH];
		float origin[3];
		float axis[3][3];
	};
	static_assert(sizeof(MD3Tag) == 112, "MD3Tag size is wrong");

	struct MD3SurfaceHeader {
		uint32_t magic;
		char name[MD3_MAX_QPATH];
		int32_t flags;
		int32_t numFrames;
		int32_t numShaders;
		int32_t numVerts;
		int32_t numTriangles;
		int32_t ofsTriangles;
		int32_t ofsShaders;
		int32_t ofsST;
		int32_t ofsXYZNormals;
		int32_t ofsEnd;
	};
	static_assert(sizeof(MD3SurfaceHeader) == 108, "MD3SurfaceHeader size is wrong");

	struct MD3Shader {
		char name[MD3_MAX_QPATH];
		int32_t shaderIndex;
	};
	static_assert(sizeof(MD3Shader) == 68, "MD3Shader size is wrong");

	struct MD3Triangle {
		int32_t indices[3];
	};
	static_assert(sizeof(MD3Triangle) == 12, "MD3Triangle size is wrong");

	struct MD3TexCoord {
		float st[2];
	};
	static_assert(sizeof(MD3TexCoord) == 8, "MD3TexCoord size is wrong");

	struct MD3Vertex {
		int16_t xyz[3];
		int16_t normal;
	};
	static_assert(sizeof(MD3Vertex) == 8, "MD3Vertex size is wrong");

private:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;
	bool loadSurface(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream &stream,
					 int64_t &surfaceStart, const MD3Header &hdr, scenegraph::SceneGraph &sceneGraph);
	bool loadTags(io::SeekableReadStream &stream, int64_t startOffset, const MD3Header &hdr,
				  scenegraph::SceneGraph &sceneGraph);

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale = glm::vec3(1.0f), bool quad = false, bool withColor = true,
					bool withTexCoords = true) override {
		return false;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Quake 3 Model", "", {"md3"}, {"IDP3"}, VOX_FORMAT_FLAG_MESH};
		return f;
	}
};

} // namespace voxelformat
