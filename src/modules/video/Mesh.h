/**
 * @file
 */

#pragma once

#include "io/IOResource.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "VertexBuffer.h"
#include "core/Vertex.h"
#include <vector>
#include <memory>
#include <unordered_map>

class aiScene;
class aiMesh;
class aiAnimation;
class aiNode;
class aiNodeAnim;

namespace Assimp {
class Importer;
}

namespace video {

class Mesh : public io::IOResource {
public:
	typedef std::vector<core::Vertex> Vertices;
	typedef std::vector<uint32_t> Indices;

private:
	struct RenderMeshData {
		uint32_t noOfIndices = 0u;
		uint32_t baseVertex = 0u;
		uint32_t baseIndex = 0u;
		uint32_t materialIndex = 0u;
	};

	struct BoneInfo {
		glm::mat4 boneOffset;
		glm::mat4 finalTransformation;
	};

	void loadTextureImages(const aiScene* scene, const std::string& filename);
	glm::vec3 calcInterpolatedScaling(float animationTime, const aiNodeAnim* nodeAnim);
	glm::mat4 calcInterpolatedRotation(float animationTime, const aiNodeAnim* nodeAnim);
	glm::vec3 calcInterpolatedPosition(float animationTime, const aiNodeAnim* nodeAnim);
	uint32_t findScaling(float animationTime, const aiNodeAnim* nodeAnim);
	uint32_t findRotation(float animationTime, const aiNodeAnim* nodeAnim);
	uint32_t findPosition(float animationTime, const aiNodeAnim* nodeAnim);
	const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string& nodeName);
	void readNodeHierarchy(const aiAnimation* animation, float animationTime, const aiNode* node, const glm::mat4& parentTransform);
	void loadBones(uint32_t neshIndex, const aiMesh* aiMesh);

	bool _readyToInit = false;

	// animation related stuff
	uint8_t _animationIndex = 0u;
	float _timeInSeconds = 0.0f;

	std::vector<RenderMeshData> _meshData;
	std::vector<image::ImagePtr> _images;
	std::vector<TexturePtr> _textures;
	Vertices _vertices;
	Indices _indices;
	VertexBuffer _vertexBuffer;
	VertexBuffer _vertexBufferNormals;
	int32_t _vertexBufferNormalsIndex = -1;
	int32_t _vertexBufferIndex = -1;

	// AABB
	glm::vec3 _aabbMins;
	glm::vec3 _aabbMaxs;

	std::unordered_map<std::string, uint32_t> _boneMapping;
	uint32_t _numBones = 0u;
	std::vector<BoneInfo> _boneInfo;
	glm::mat4 _globalInverseTransform;
	const aiScene* _scene = nullptr;
	Assimp::Importer* _importer;
	void* _lastShader = nullptr;
	std::string _filename;

	void setupNormalBufferAttributes(Shader& shader);
	void setupBufferAttributes(Shader& shader);
	void boneTransform(float timeInSeconds, glm::mat4* transforms, size_t size, uint8_t animationIndex = 0u);
public:
	Mesh();
	~Mesh();

	const glm::vec3& mins() const;
	const glm::vec3& maxs() const;

	const std::string& filename() const;

	const Vertices& vertices() const;
	const Indices& indices() const;

	void shutdown();
	bool loadMesh(const std::string& filename);
	bool initMesh(Shader& shader, float timeInSeconds = 0.0f, uint8_t animationIndex = 0u);
	int render();
	int renderNormals(video::Shader& shader);
};

inline const Mesh::Vertices& Mesh::vertices() const {
	return _vertices;
}

inline const Mesh::Indices& Mesh::indices() const {
	return _indices;
}

inline const std::string& Mesh::filename() const {
	return _filename;
}

inline const glm::vec3& Mesh::mins() const {
	return _aabbMins;
}

inline const glm::vec3& Mesh::maxs() const {
	return _aabbMaxs;
}

typedef std::shared_ptr<Mesh> MeshPtr;

}
