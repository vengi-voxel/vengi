/**
 * @file
 */

#pragma once

#include "core/collection/StringMap.h"
#include "voxel/Face.h"
#include "voxelformat/private/mesh/MeshFormat.h"

namespace voxel {
class RawVolumeWrapper;
}

namespace voxelformat {

/**
 * @brief Blockbench bbmodel json format
 *
 * @ingroup Formats
 */
class BlockbenchFormat : public MeshFormat {
public:
	struct Face {
		glm::vec2 uvs[2]{{0.0f, 1.0f}, {0.0f, 1.0f}};
		int textureIndex = -1;
	};

	struct Cube {
		Face faces[(int)voxel::FaceNames::Max];
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

	struct Element {
		core::String uuid;
		core::String name;
		// in degrees
		glm::vec3 rotation{0.0f};
		// in world coordinates
		glm::vec3 origin{0.0f}; // pivot - not normalized
		glm::vec3 from{0.0f};
		glm::vec3 to{0.0f};
		Cube cube;
	};

	// map via uuid
	using ElementMap = core::StringMap<Element>;

	using Textures = core::DynamicArray<image::ImagePtr>;

private:
	void fillFace(scenegraph::SceneGraphNode &node, voxel::FaceNames faceName, const image::ImagePtr &image,
				  const glm::vec2 &uv0, const glm::vec2 &uv1) const;
	bool addNode(const Node &node, const ElementMap &elementMap, scenegraph::SceneGraph &sceneGraph,
				 const Textures &textureArray, int parent) const;
	bool generateVolumeFromElement(const Node &node, const Element &element, const Textures &textureArray,
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
