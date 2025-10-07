/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "MeshMaterial.h"
#include "core/collection/Buffer.h"
#include "core/collection/StringMap.h"
#include "util/Version.h"
#include "voxel/Face.h"

namespace voxel {
class RawVolumeWrapper;
}

namespace voxelformat {

/**
 * @brief Blockbench bbmodel json format
 *
 * In blockbench a scene is built by elements (volumes) and nodes in the outliner (groups).
 * The elements are referenced by the nodes.
 * A cube is made by 6 faces with uv coordinates and a texture index. A voxel (1x1x1 cube in blockbench) can therefore
 * have 6 different colors. This is not possible in vengi. We therefore define an order of faces where one overwrites
 * the other. This order is hardcoded from negative to positive from x to y to z. This basically means that the front
 * and the back sides are the dominant ones - followed by the up and down sides - and finally the left and right sides.
 *
 * @ingroup Formats
 */
class BlockbenchFormat : public MeshFormat {
public:
	struct BBCubeFace {
		glm::vec2 uvs[2]{{0.0f, 1.0f}, {0.0f, 1.0f}};
		int textureIndex = -1;
	};

	struct BBCube {
		BBCubeFace faces[(int)voxel::FaceNames::Max];
		glm::vec3 from{0.0f};
		glm::vec3 to{0.0f};
	};

	struct BBNode {
		core::UUID uuid;
		core::String name;
		// in degrees
		glm::vec3 rotation{0.0f};
		// in world coordinates
		glm::vec3 origin{0.0f};
		bool locked = false;
		bool visible = true;
		bool mirror_uv = false;
		int color = 0;
		// group nodes
		core::DynamicArray<BBNode> children;
		// elements (volumes) by uuid
		core::DynamicArray<core::UUID> referenced;
	};

	enum class BBElementType { Cube, Mesh, Max };

	struct BBElement {
		core::UUID uuid;
		core::String name;
		// in degrees
		glm::vec3 rotation{0.0f};
		// in world coordinates
		glm::vec3 origin{0.0f}; // pivot - not normalized
		bool rescale = false;
		bool locked = false;
		bool box_uv = false;
		int color = 0;
		BBCube cube;
		Mesh mesh;
		BBElementType type = BBElementType::Max;
	};

	struct BBMeta {
		// 1654934558
		uint64_t creationTimestamp = 0;
		bool box_uv = false;
		util::Version version{0, 0};
		// model_format: free bedrock bedrock_old java_block animated_entity_model skin
		core::String modelFormat;
		core::String formatVersion;
		core::String name;
		core::String model_identifier;
		glm::ivec2 resolution;
	};

	// map via uuid
	using BBElementMap = core::Map<core::UUID, BBElement, 11, core::UUIDHash>;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Blockbench", {"bbmodel"}, {}, VOX_FORMAT_FLAG_MESH};
		return f;
	}

private:
	bool addNode(const BBNode &node, const BBElementMap &elementMap, scenegraph::SceneGraph &sceneGraph,
				 const MeshMaterialArray &meshMaterialArray, int parent) const;
	bool generateCube(const BBNode &node, const BBElement &element, const MeshMaterialArray &meshMaterialArray,
					  scenegraph::SceneGraph &sceneGraph, int parent) const;
	bool generateMesh(const BBNode &node, BBElement &element, const MeshMaterialArray &meshMaterialArray,
					  scenegraph::SceneGraph &sceneGraph, int parent) const;

protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override {
		return false;
	}
};

} // namespace voxelformat
