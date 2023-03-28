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
struct Animation;
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
	void saveGltfNode(core::Map<int, int> &nodeMapping, tinygltf::Model &gltfModel, tinygltf::Node &gltfNode,
					  tinygltf::Scene &gltfScene, const scenegraph::SceneGraphNode &graphNode, Stack &stack,
					  const scenegraph::SceneGraph &sceneGraph, const glm::vec3 &scale, bool exportAnimations);

	void saveAnimation(int targetNode, tinygltf::Model &m, const scenegraph::SceneGraphNode &node,
					   tinygltf::Animation &gltfAnimation);

	// importing (voxelization)
	struct GltfVertex {
		glm::vec3 pos{0.0f};
		glm::vec2 uv{0.0f};
		image::TextureWrap wrapS = image::TextureWrap::Repeat;
		image::TextureWrap wrapT = image::TextureWrap::Repeat;
		core::RGBA color{0};
		core::String texture;
	};
	struct GltfTextureData {
		core::String diffuseTexture;
		core::String texCoordAttribute;
		image::TextureWrap wrapS = image::TextureWrap::Repeat;
		image::TextureWrap wrapT = image::TextureWrap::Repeat;
	};
	bool loadTextures(const core::String &filename, core::StringMap<image::ImagePtr> &textures,
					  const tinygltf::Model &gltfModel, const tinygltf::Primitive &gltfPrimitive,
					  GltfTextureData &textureData) const;
	bool loadAttributes(const core::String &filename, core::StringMap<image::ImagePtr> &textures,
						const tinygltf::Model &gltfModel, const tinygltf::Primitive &gltfPrimitive,
						core::DynamicArray<GltfVertex> &vertices) const;

	bool loadAnimations(scenegraph::SceneGraph &sceneGraph, const tinygltf::Model &model, int gltfNodeIdx,
						scenegraph::SceneGraphNode &node) const;
	bool loadNode_r(const core::String &filename, scenegraph::SceneGraph &sceneGraph,
					core::StringMap<image::ImagePtr> &textures, const tinygltf::Model &gltfModel, int gltfNodeIdx,
					int parentNodeId) const;
	bool loadIndices(const tinygltf::Model &model, const tinygltf::Primitive &gltfPrimitive,
					 core::DynamicArray<uint32_t> &indices, size_t indicesOffset) const;
	scenegraph::SceneGraphTransform loadTransform(const tinygltf::Node &gltfNode) const;
	size_t accessorSize(const tinygltf::Accessor &gltfAccessor) const;
	const tinygltf::Accessor *getAccessor(const tinygltf::Model &gltfModel, int id) const;

	bool subdivideShape(scenegraph::SceneGraphNode &node, const TriCollection &tris, const glm::vec3 &offset,
						bool axisAlignedMesh) const;
	bool voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;

public:
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override;
};

} // namespace voxelformat
