/**
 * @file
 */

#pragma once

#include "MeshExporter.h"

namespace tinygltf {
class Model;
class Node;
class Scene;
} // namespace tinygltf

namespace voxel {

/**
 * @brief GL Transmission Format
 */
class GLTFFormat : public MeshExporter {
private:
	struct Pair {
		constexpr Pair(int f, int s) : first(f), second(s) {
		}
		int first;
		int second;
	};
	typedef core::DynamicArray<Pair> Stack;
	core::Map<int, int> meshIdxNodeMap;
	void processGltfNode(tinygltf::Model &m, tinygltf::Node &node, tinygltf::Scene &scene,
						 const voxel::SceneGraphNode &graphNode, Stack &stack);
	bool saveMeshes(const SceneGraph &sceneGraph, const Meshes &meshes, const core::String &filename,
					io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad, bool withColor,
					bool withTexCoords);
	bool saveMeshes(const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override;

public:
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream) override;
};

} // namespace voxel
