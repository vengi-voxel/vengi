/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "voxel/Face.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelformat/private/mesh/TexturedTri.h"

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
	struct CubeFace {
		glm::vec2 uvs[2]{{0.0f, 1.0f}, {0.0f, 1.0f}};
		int textureIndex = -1;
	};

	struct Cube {
		CubeFace faces[(int)voxel::FaceNames::Max];
		glm::vec3 from{0.0f};
		glm::vec3 to{0.0f};
	};

	struct Mesh {
		MeshFormat::TriCollection tris;
	};

	struct Node {
		core::String uuid;
		core::String name;
		// in degrees
		glm::vec3 rotation{0.0f};
		// in world coordinates
		glm::vec3 origin{0.0f};
		bool locked = false;
		bool visible = true;
		// group nodes
		core::DynamicArray<Node> children;
		// elements (volumes) by uuid
		core::DynamicArray<core::String> referenced;
	};

	enum class ElementType { Cube, Mesh, Max };

	struct Element {
		core::String uuid;
		core::String name;
		// in degrees
		glm::vec3 rotation{0.0f};
		// in world coordinates
		glm::vec3 origin{0.0f}; // pivot - not normalized
		Cube cube;
		Mesh mesh;
		ElementType type = ElementType::Max;
	};

	// map via uuid
	using ElementMap = core::StringMap<Element>;

	using Textures = core::DynamicArray<image::ImagePtr>;

private:
	bool addNode(const Node &node, const ElementMap &elementMap, scenegraph::SceneGraph &sceneGraph,
				 const Textures &textureArray, int parent) const;
	bool generateCube(const Node &node, const Element &element, const Textures &textureArray,
					  scenegraph::SceneGraph &sceneGraph, int parent) const;
	bool generateMesh(const Node &node, const Element &element, const Textures &textureArray,
					  scenegraph::SceneGraph &sceneGraph, int parent) const;

protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const Meshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override {
		return false;
	}
};

} // namespace voxelformat
