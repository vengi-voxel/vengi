/**
 * @file
 */

#pragma once

#include "voxelformat/private/mesh/MeshFormat.h"

namespace voxelformat {
/**
 * @brief Quake1 PolyModel format
 * @link http://tfc.duke.free.fr/coding/mdl-specs-en.html
 * @link https://formats.kaitai.io/quake_mdl/
 * @link https://book.leveldesignbook.com/appendix/resources/formats/mdl
 *
 * @ingroup Formats
 */
class MDLFormat : public MeshFormat {
public:
	struct MDLHeader {
		uint32_t magic = 0;
		uint32_t version = 0;
		glm::vec3 scale{};
		glm::vec3 origin{};
		float radius = 0.0f; /* the radius of a sphere covering the whole model (collision detection) */

		glm::vec3 eye{}; /* where the eyes are located in the model */

		uint32_t numSkins = 0;
		// all textures must have the same dimensions
		uint32_t skinWidth = 0;
		uint32_t skinHeight = 0;

		uint32_t numVerts = 0; /* number of vertices for one frame */
		uint32_t numTris = 0;
		uint32_t numFrames = 0;
		uint32_t synctype = 0; /* 0 = synchron, 1 = random */
		uint32_t flags = 0;
		float size = 0.0f;

		uint32_t numTexCoords = -1; // raven polymodel format
	};
	static_assert(sizeof(MDLHeader) == 88, "MDLHeader size is wrong");

private:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale = glm::vec3(1.0f), bool quad = false, bool withColor = true,
					bool withTexCoords = true) override {
		return false;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Quake 1 Model", "", {"mdl"}, {"IDPO", "RAPO"}, VOX_FORMAT_FLAG_MESH};
		return f;
	}
};
} // namespace voxelformat
