/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "core/Pair.h"
#include "core/collection/StringMap.h"

namespace tinygltf {
class Model;
class Node;
struct Scene;
struct Material;
struct Primitive;
struct Accessor;
} // namespace tinygltf

namespace scenegraph {
class SceneGraphTransform;
}
namespace voxelformat {

/**
 * @brief GL Transmission Format
 * https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/figures/gltfOverview-2.0.0b.png
 *
 * @li Viewer including animations: https://sandbox.babylonjs.com/
 * @li GLTF-Validator: https://github.khronos.org/glTF-Validator/
 *
 * @ingroup Formats
 */
class GLTFFormat : public MeshFormat {
private:
	// exporting
	typedef core::DynamicArray<core::Pair<int, int>> Stack;
	void processGltfNode(tinygltf::Model &m, tinygltf::Node &node, tinygltf::Scene &scene,
						 const scenegraph::SceneGraphNode &graphNode, Stack &stack,
						 const scenegraph::SceneGraph &sceneGraph, const glm::vec3 &scale, bool exportAnimations);

	// importing (voxelization)
	struct GltfVertex {
		glm::vec3 pos{0.0f};
		glm::vec2 uv{0.0f};
		image::TextureWrap wrapS = image::TextureWrap::Repeat;
		image::TextureWrap wrapT = image::TextureWrap::Repeat;
		core::RGBA color{0};
		core::String texture;
	};
	bool loadGlftAttributes(const core::String &filename, core::StringMap<image::ImagePtr> &textures,
							const tinygltf::Model &model, const tinygltf::Primitive &primitive,
							core::DynamicArray<GltfVertex> &vertices) const;

	bool loadGltfAnimations(scenegraph::SceneGraph &sceneGraph, const tinygltf::Model &model, int &gltfNodeIdx,
							scenegraph::SceneGraphNode &node) const;
	bool loadGltfNode_r(const core::String &filename, scenegraph::SceneGraph &sceneGraph,
						core::StringMap<image::ImagePtr> &textures, const tinygltf::Model &model, int gltfNodeIdx,
						int parentNodeId) const;
	bool loadGltfIndices(const tinygltf::Model &model, const tinygltf::Primitive &primitive,
						 core::DynamicArray<uint32_t> &indices, size_t indicesOffset) const;
	scenegraph::SceneGraphTransform loadGltfTransform(const tinygltf::Node &gltfNode) const;
	size_t getGltfAccessorSize(const tinygltf::Accessor &accessor) const;
	const tinygltf::Accessor *getGltfAccessor(const tinygltf::Model &model, int id) const;

	bool subdivideShape(scenegraph::SceneGraphNode &node, const TriCollection &tris, const glm::vec3 &offset,
						bool axisAlignedMesh) const;

	void processAnimation(tinygltf::Model &m, tinygltf::Scene &scene, const scenegraph::SceneGraphNode &graphNode,
						  const scenegraph::SceneGraph &sceneGraph, const core::String &animationId);

	bool voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override;
};

} // namespace voxelformat
