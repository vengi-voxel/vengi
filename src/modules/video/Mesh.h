/**
 * @file
 */

#pragma once

#include "io/IOResource.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "video/GLMeshData.h"
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <memory>
#include <unordered_map>

namespace video {

class Mesh : public io::IOResource {
private:
#define NUM_BONES_PER_VEREX 4
	struct BoneInfo {
		glm::mat4 boneOffset;
		glm::mat4 finalTransformation;
	};

	struct Vertex {
		glm::vec3 _pos;
		glm::vec3 _norm;
		glm::vec2 _uv;
		glm::vec4 _color;
		uint32_t _boneIds[NUM_BONES_PER_VEREX];
		float _boneWeights[NUM_BONES_PER_VEREX];

		Vertex(const glm::vec3& p, const glm::vec3& n, const glm::vec2& t, const glm::vec4& c) :
				_pos(p), _norm(n), _uv(t), _color(c), _boneIds { 0u, 0u, 0u, 0u }, _boneWeights { 0.0f, 0.0f, 0.0f, 0.0f } {
		}

		Vertex(const aiVector3D& p, const aiVector3D& n, const aiVector3D& t, const aiColor4D& c) :
				Vertex(glm::vec3(p.x, p.y, p.z), glm::vec3(n.x, n.y, n.z), glm::vec2(t.x, t.y), glm::vec4(c.r, c.g, c.b, c.a)) {
		}

		void addBoneData(uint32_t boneID, float weight);
	};
	typedef std::vector<Vertex> Vertices;
	typedef std::vector<uint32_t> Indices;

	glm::mat4 toMat4(const aiMatrix4x4& matrix) const;
	glm::mat4 toMat4(const aiMatrix3x3& matrix) const;
	void loadTextureImages(const aiScene* scene, const std::string& filename);
	void calcInterpolatedScaling(aiVector3D& out, float animationTime, const aiNodeAnim* nodeAnim);
	void calcInterpolatedRotation(aiQuaternion& out, float animationTime, const aiNodeAnim* nodeAnim);
	void calcInterpolatedPosition(aiVector3D& out, float animationTime, const aiNodeAnim* nodeAnim);
	uint32_t findScaling(float animationTime, const aiNodeAnim* nodeAnim);
	uint32_t findRotation(float animationTime, const aiNodeAnim* nodeAnim);
	uint32_t findPosition(float animationTime, const aiNodeAnim* nodeAnim);
	const aiNodeAnim* findNodeAnim(const aiAnimation* animation, const std::string& nodeName);
	void readNodeHierarchy(const aiAnimation* animation, float animationTime, const aiNode* node, const glm::mat4& parentTransform);
	void loadBones(uint32_t neshIndex, const aiMesh* aiMesh);

	GLuint _vertexArrayObject = 0u;
	GLuint _vbo = 0u;
	GLuint _indexBuffer = 0u;
	bool _readyToInit = false;

	std::vector<GLMeshData> _meshData;
	std::vector<image::ImagePtr> _images;
	std::vector<TexturePtr> _textures;
	Vertices _vertices;
	Indices _indices;
	std::unordered_map<std::string, uint32_t> _boneMapping;
	uint32_t _numBones = 0u;
	std::vector<BoneInfo> _boneInfo;
	glm::mat4 _globalInverseTransform;
	const aiScene* _scene = nullptr;
	Assimp::Importer _importer;
	void* _lastShader = nullptr;

	void boneTransform(float timeInSeconds, std::vector<glm::mat4>& transforms, uint8_t animationIndex = 0u);
public:
	Mesh();
	~Mesh();

	void shutdown();
	bool loadMesh(const std::string& filename);
	bool initMesh(Shader& shader, float timeInSeconds = 0.0f, uint8_t animationIndex = 0u);
	int render();
};

typedef std::shared_ptr<Mesh> MeshPtr;

}
