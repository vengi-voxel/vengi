/**
 * @file
 */

#pragma once

#include "io/IOResource.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "video/Buffer.h"
#include "core/Vertex.h"
#include "core/Color.h"
#include <vector>
#include <memory>
#include <unordered_map>

struct aiScene;
struct aiMesh;
struct aiAnimation;
struct aiNode;
struct aiNodeAnim;

namespace Assimp {
class Importer;
}

namespace video {

/**
 * @ingroup Video
 */
class Mesh : public io::IOResource {
public:
	typedef std::vector<core::Vertex> Vertices;
	typedef std::vector<uint32_t> Indices;

private:
	struct MeshLines {
		struct AttributeData {
			glm::vec4 vertex;
			glm::vec3 color {core::Color::Red};
		};
		std::vector<AttributeData> data;

		inline void reserve(size_t amount) {
			data.resize(amount);
		}
	};

	struct RenderMeshData {
		uint32_t noOfIndices = 0u;
		uint32_t baseVertex = 0u;
		uint32_t baseIndex = 0u;
		uint32_t materialIndex = 0u;
	};

	struct BoneInfo {
		glm::mat4 boneOffset {1.0f};
		glm::mat4 finalTransformation {1.0f};
	};

	void loadTextureImages(const aiScene* scene, const std::string& dir, const std::string& basename);
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
	Buffer _vertexBuffer;
	Buffer _vertexBufferLines;
	int32_t _vertexBufferLinesIndex = -1;
	int32_t _vertexBufferIndex = -1;

	// AABB
	glm::vec3 _aabbMins;
	glm::vec3 _aabbMaxs;

	glm::vec3 _scale { 1.0f };

	std::unordered_map<std::string, uint32_t> _boneMapping;
	uint32_t _numBones = 0u;
	std::vector<BoneInfo> _boneInfo;
	glm::mat4 _globalInverseTransform;
	const aiScene* _scene = nullptr;
	Assimp::Importer* _importer;
	void* _lastShader = nullptr;
	std::string _filename;

	void setupLineBufferAttributes(Shader& shader);
	void setupBufferAttributes(Shader& shader);
	void boneTransform(glm::mat4* transforms, size_t size);
	/**
	 * @brief We render the bone data as joint lines with a start position and the end position
	 */
	void traverseBones(MeshLines& boneData, const aiNode* node, const glm::mat4& parent, const glm::vec3& start, bool traverse = false);
public:
	Mesh();
	~Mesh();

	const glm::vec3& mins() const;
	const glm::vec3& maxs() const;

	const std::string& filename() const;

	const Vertices& vertices() const;
	const Indices& indices() const;

	void setScale(const glm::vec3& scale);
	const glm::vec3& scale() const;
	glm::vec3& scale();

	int bones() const;
	int animations() const;
	uint8_t currentAnimation() const;
	void shutdown();
	bool loadMesh(const std::string& filename);
	bool initMesh(Shader& shader, float timeInSeconds = 0.0f, uint8_t animationIndex = 0u);
	int render();
	int renderNormals(video::Shader& shader);
	int renderBones(video::Shader& shader);
};

inline void Mesh::setScale(const glm::vec3& scale) {
	_scale = scale;
}

inline const glm::vec3& Mesh::scale() const {
	return _scale;
}

inline glm::vec3& Mesh::scale() {
	return _scale;
}

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

inline int Mesh::bones() const {
	return _numBones;
}

inline uint8_t Mesh::currentAnimation() const {
	return _animationIndex;
}

typedef std::shared_ptr<Mesh> MeshPtr;

}
