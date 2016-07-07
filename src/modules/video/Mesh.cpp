/**
 * @file
 */

#include "Mesh.h"
#include "GLFunc.h"
#include "core/Common.h"
#include "core/Log.h"

namespace video {

namespace {
const aiVector3D VECZERO(0.0f, 0.0f, 0.0f);
}

Mesh::Mesh() :
		io::IOResource() {
}

Mesh::~Mesh() {
	core_assert_msg(_vertexArrayObject == 0u, "Mesh was not properly shut down");
	shutdown();
}

void Mesh::shutdown() {
	_textures.clear();
	_images.clear();
	_meshData.clear();

	_vertices.clear();
	_indices.clear();

	_readyToInit = false;
	if (_vbo != 0u) {
		glDeleteBuffers(1, &_vbo);
		_vbo = 0u;
	}
	if (_indexBuffer != 0u) {
		glDeleteBuffers(1, &_indexBuffer);
		_indexBuffer = 0u;
	}
	if (_vertexArrayObject != 0u) {
		glDeleteVertexArrays(1, &_vertexArrayObject);
		_vertexArrayObject = 0u;
	}
}

bool Mesh::loadMesh(const std::string& filename) {
	Assimp::Importer importer;
#if 0
	// TODO: implement custom io handler to support meshes that are split over several files (like obj)
	class MeshIOSystem : public Assimp::IOSystem {
	};
	MeshIOSystem iosystem;
	importer.SetIOHandler(&iosystem);
#endif
	const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_FindDegenerates);
	if (scene == nullptr || scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		Log::error("Error parsing '%s': '%s'\n", filename.c_str(), importer.GetErrorString());
		_state = io::IOSTATE_FAILED;
		return false;
	}

	_globalInverseTransform = glm::inverse(toMat4(scene->mRootNode->mTransformation));

	_meshData.resize(scene->mNumMeshes);

	uint32_t numIndices = 0u;
	uint32_t numVertices = 0u;

	for (uint32_t i = 0; i < _meshData.size(); ++i) {
		const aiMesh* mesh = scene->mMeshes[i];
		GLMeshData& meshData = _meshData[i];
		meshData.materialIndex = mesh->mMaterialIndex;
		meshData.noOfIndices = mesh->mNumFaces * 3;
		meshData.baseVertex = numVertices;
		meshData.baseIndex = numIndices;
		meshData.indexType = GL_UNSIGNED_INT;

		numVertices += mesh->mNumVertices;
		numIndices += meshData.noOfIndices;
	}

	_bones.reserve(numVertices);
	_vertices.reserve(numVertices);
	_indices.reserve(numIndices);

	for (uint32_t i = 0; i < _meshData.size(); i++) {
		const aiMesh* mesh = scene->mMeshes[i];
		for (uint32_t i = 0; i < mesh->mNumFaces; ++i) {
			const aiFace& face = mesh->mFaces[i];
			core_assert(face.mNumIndices == 3);
			_indices.push_back(face.mIndices[0]);
			_indices.push_back(face.mIndices[1]);
			_indices.push_back(face.mIndices[2]);
		}

		for (uint32_t i = 0; i < mesh->mNumVertices; ++i) {
			const aiVector3D* pos = &mesh->mVertices[i];
			const aiVector3D* normal = &mesh->mNormals[i];
			const aiVector3D* texCoord = mesh->HasTextureCoords(0) ? &mesh->mTextureCoords[0][i] : &VECZERO;

			_vertices.emplace_back(glm::vec3(pos->x, pos->y, pos->z), glm::vec3(normal->x, normal->y, normal->z), glm::vec2(texCoord->x, texCoord->y));
		}
		loadBones(i, mesh, _bones);
	}

	loadTextureImages(scene, filename);
	_readyToInit = true;
	Log::info("Loaded mesh %s with %i vertices and %i indices", filename.c_str(), (int)_vertices.size(), (int)_indices.size());
	return true;
}

bool Mesh::initMesh(Shader& shader) {
	if (_state != io::IOSTATE_LOADED) {
		if (!_readyToInit) {
			return false;
		}

		for (const image::ImagePtr& i : _images) {
			if (i && i->isLoading()) {
				return false;
			}
		}

		glGenVertexArrays(1, &_vertexArrayObject);
		// generate all the 4 needed buffers at once
		glGenBuffers(1, &_vbo);
		glGenBuffers(1, &_indexBuffer);

		_textures.resize(_images.size());
		int materialIndex = 0;
		for (const image::ImagePtr& i : _images) {
			if (i && i->isLoaded()) {
				_textures[materialIndex++] = createTextureFromImage(i);
			} else {
				++materialIndex;
			}
		}
		_images.clear();

		_state = io::IOSTATE_LOADED;
	} else {
		return true;
	}

	glBindVertexArray(_vertexArrayObject);

	glBindBuffer(GL_ARRAY_BUFFER, _vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * _vertices.size(), &_vertices[0], GL_STATIC_DRAW);
	if (shader.hasAttribute("a_pos")) {
		const int loc = shader.enableVertexAttribute("a_pos");
		core_assert(loc >= 0);
		glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), GL_OFFSET_CAST(offsetof(Vertex, _pos)));
	}
	if (shader.hasAttribute("a_texcoords")) {
		const int loc = shader.enableVertexAttribute("a_texcoords");
		core_assert(loc >= 0);
		glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), GL_OFFSET_CAST(offsetof(Vertex, _uv)));
	}
	if (shader.hasAttribute("a_norm")) {
		const int loc = shader.enableVertexAttribute("a_norm");
		core_assert(loc >= 0);
		glVertexAttribPointer(loc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), GL_OFFSET_CAST(offsetof(Vertex, _norm)));
	}

#if 0
	// TODO: move bones data into the vbo
	glBindBuffer(GL_ARRAY_BUFFER, m_Buffers[BONE_VB]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(_bones[0]) * _bones.size(), &_bones[0], GL_STATIC_DRAW);
#endif
	if (shader.hasAttribute("a_boneid")) {
		const int loc = shader.enableVertexAttribute("a_boneid");
		core_assert(loc >= 0);
		glVertexAttribPointer(loc, 4, GL_INT, GL_FALSE, sizeof(VertexBoneData), GL_OFFSET_CAST(offsetof(VertexBoneData, boneIds)));
	}
	if (shader.hasAttribute("a_boneweight")) {
		const int loc = shader.enableVertexAttribute("a_boneweight");
		core_assert(loc >= 0);
		glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData), GL_OFFSET_CAST(offsetof(VertexBoneData, boneWeights)));
	}

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(_indices[0]) * _indices.size(), &_indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);

	return GL_checkError() == 0;
}

void Mesh::VertexBoneData::AddBoneData(uint32_t boneID, float weight) {
	for (uint i = 0; i < SDL_arraysize(boneIds); i++) {
		// TODO: fabs please - bobs
		if (boneWeights[i] == 0.0) {
			boneIds[i] = boneID;
			boneWeights[i] = weight;
			return;
		}
	}

	// should never get here - more bones than we have space for
	core_assert(false);
}

glm::mat4 Mesh::toMat4(const aiMatrix4x4& matrix) const {
#if 0
	// TODO:
	GLM_FUNC_DECL tmat4x4(
		X1 const & x1, Y1 const & y1, Z1 const & z1, W1 const & w1,
		X2 const & x2, Y2 const & y2, Z2 const & z2, W2 const & w2,
		X3 const & x3, Y3 const & y3, Z3 const & z3, W3 const & w3,
		X4 const & x4, Y4 const & y4, Z4 const & z4, W4 const & w4);
#endif
	return glm::mat4();
}

glm::mat4 Mesh::toMat4(const aiMatrix3x3& matrix) const {
#if 0
	// TODO:
	GLM_FUNC_DECL tmat4x4(
		X1 const & x1, Y1 const & y1, Z1 const & z1, W1 const & w1,
		X2 const & x2, Y2 const & y2, Z2 const & z2, W2 const & w2,
		X3 const & x3, Y3 const & y3, Z3 const & z3, W3 const & w3,
		X4 const & x4, Y4 const & y4, Z4 const & z4, W4 const & w4);
#endif
	return glm::mat4();
}

void Mesh::loadBones(uint32_t meshIndex, const aiMesh* mesh, std::vector<VertexBoneData>& bones) {
	for (uint32_t i = 0; i < mesh->mNumBones; i++) {
		uint32_t boneIndex = 0;
		const std::string boneName(mesh->mBones[i]->mName.data);

		if (_boneMapping.find(boneName) == _boneMapping.end()) {
			// Allocate an index for a new bone
			boneIndex = _numBones;
			_numBones++;
			BoneInfo bi;
			_boneInfo.push_back(bi);
			_boneInfo[boneIndex].boneOffset = toMat4(mesh->mBones[i]->mOffsetMatrix);
			_boneMapping[boneName] = boneIndex;
		} else {
			boneIndex = _boneMapping[boneName];
		}

		for (uint32_t j = 0; j < mesh->mBones[i]->mNumWeights; j++) {
			const uint32_t vertexID = _meshData[meshIndex].baseVertex + mesh->mBones[i]->mWeights[j].mVertexId;
			const float weight = mesh->mBones[i]->mWeights[j].mWeight;
			bones[vertexID].AddBoneData(boneIndex, weight);
		}
	}
}

uint32_t Mesh::findPosition(float animationTime, const aiNodeAnim* nodeAnim) {
	for (uint32_t i = 0; i < nodeAnim->mNumPositionKeys - 1; i++) {
		if (animationTime < (float) nodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}
	}

	core_assert(false);

	return 0;
}

uint32_t Mesh::findRotation(float animationTime, const aiNodeAnim* nodeAnim) {
	core_assert(nodeAnim->mNumRotationKeys > 0);

	for (uint32_t i = 0; i < nodeAnim->mNumRotationKeys - 1; i++) {
		if (animationTime < (float) nodeAnim->mRotationKeys[i + 1].mTime) {
			return i;
		}
	}

	core_assert(false);

	return 0;
}

uint Mesh::findScaling(float animationTime, const aiNodeAnim* nodeAnim) {
	core_assert(nodeAnim->mNumScalingKeys > 0);

	for (uint i = 0; i < nodeAnim->mNumScalingKeys - 1; i++) {
		if (animationTime < (float) nodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}

	core_assert(false);

	return 0;
}

void Mesh::calcInterpolatedPosition(aiVector3D& out, float animationTime, const aiNodeAnim* nodeAnim) {
	if (nodeAnim->mNumPositionKeys == 1) {
		out = nodeAnim->mPositionKeys[0].mValue;
		return;
	}

	const uint32_t positionIndex = findPosition(animationTime, nodeAnim);
	const uint32_t nextPositionIndex = (positionIndex + 1);
	core_assert(nextPositionIndex < nodeAnim->mNumPositionKeys);
	const float deltaTime = (float) (nodeAnim->mPositionKeys[nextPositionIndex].mTime - nodeAnim->mPositionKeys[positionIndex].mTime);
	const float factor = (animationTime - (float) nodeAnim->mPositionKeys[positionIndex].mTime) / deltaTime;
	core_assert(factor >= 0.0f && factor <= 1.0f);
	const aiVector3D& start = nodeAnim->mPositionKeys[positionIndex].mValue;
	const aiVector3D& end = nodeAnim->mPositionKeys[nextPositionIndex].mValue;
	const aiVector3D delta = end - start;
	out = start + factor * delta;
}

void Mesh::calcInterpolatedRotation(aiQuaternion& out, float animationTime, const aiNodeAnim* nodeAnim) {
	// we need at least two values to interpolate...
	if (nodeAnim->mNumRotationKeys == 1) {
		out = nodeAnim->mRotationKeys[0].mValue;
		return;
	}

	const uint32_t rotationIndex = findRotation(animationTime, nodeAnim);
	const uint32_t nextRotationIndex = (rotationIndex + 1);
	core_assert(nextRotationIndex < nodeAnim->mNumRotationKeys);
	const float deltaTime = (float) (nodeAnim->mRotationKeys[nextRotationIndex].mTime - nodeAnim->mRotationKeys[rotationIndex].mTime);
	const float factor = (animationTime - (float) nodeAnim->mRotationKeys[rotationIndex].mTime) / deltaTime;
	core_assert(factor >= 0.0f && factor <= 1.0f);
	const aiQuaternion& startRotationQ = nodeAnim->mRotationKeys[rotationIndex].mValue;
	const aiQuaternion& endRotationQ = nodeAnim->mRotationKeys[nextRotationIndex].mValue;
	aiQuaternion::Interpolate(out, startRotationQ, endRotationQ, factor);
	out = out.Normalize();
}

void Mesh::calcInterpolatedScaling(aiVector3D& out, float animationTime, const aiNodeAnim* nodeAnim) {
	if (nodeAnim->mNumScalingKeys == 1) {
		out = nodeAnim->mScalingKeys[0].mValue;
		return;
	}

	const uint32_t scalingIndex = findScaling(animationTime, nodeAnim);
	const uint32_t nextScalingIndex = (scalingIndex + 1);
	core_assert(nextScalingIndex < nodeAnim->mNumScalingKeys);
	const float deltaTime = (float) (nodeAnim->mScalingKeys[nextScalingIndex].mTime
			- nodeAnim->mScalingKeys[scalingIndex].mTime);
	const float factor = (animationTime - (float) nodeAnim->mScalingKeys[scalingIndex].mTime) / deltaTime;
	core_assert(factor >= 0.0f && factor <= 1.0f);
	const aiVector3D& start = nodeAnim->mScalingKeys[scalingIndex].mValue;
	const aiVector3D& end = nodeAnim->mScalingKeys[nextScalingIndex].mValue;
	const aiVector3D delta = end - start;
	out = start + factor * delta;
}

void Mesh::readNodeHierarchy(aiScene* scene, float animationTime, const aiNode* node, const glm::mat4& parentTransform) {
	// TODO: what about scene->mNumAnimations
	const std::string nodeName(node->mName.data);
	const aiAnimation* animation = scene->mAnimations[0];
	glm::mat4 nodeTransformation;
	const aiNodeAnim* nodeAnim = findNodeAnim(animation, nodeName);

	if (nodeAnim != nullptr) {
		// Interpolate scaling and generate scaling transformation matrix
		aiVector3D scaling;
		calcInterpolatedScaling(scaling, animationTime, nodeAnim);
		const glm::mat4 scalingM = glm::scale(glm::mat4(), glm::vec3(scaling.x, scaling.y, scaling.z));

		// Interpolate rotation and generate rotation transformation matrix
		aiQuaternion rotationQ;
		calcInterpolatedRotation(rotationQ, animationTime, nodeAnim);
		const glm::mat4 rotationM = glm::mat4(toMat4(rotationQ.GetMatrix()));

		// Interpolate translation and generate translation transformation matrix
		aiVector3D translation;
		calcInterpolatedPosition(translation, animationTime, nodeAnim);
		const glm::mat4 translationM = glm::translate(glm::mat4(), glm::vec3(translation.x, translation.y, translation.z));

		// Combine the above transformations
		nodeTransformation = translationM * rotationM * scalingM;
	} else {
		nodeTransformation = toMat4((node->mTransformation));
	}

	const glm::mat4 globalTransformation = parentTransform * nodeTransformation;

	if (_boneMapping.find(nodeName) != _boneMapping.end()) {
		const uint32_t boneIndex = _boneMapping[nodeName];
		_boneInfo[boneIndex].finalTransformation = _globalInverseTransform * globalTransformation * _boneInfo[boneIndex].boneOffset;
	}

	for (uint32_t i = 0; i < node->mNumChildren; i++) {
		readNodeHierarchy(scene, animationTime, node->mChildren[i], globalTransformation);
	}
}

void Mesh::boneTransform(aiScene* scene, float timeInSeconds, std::vector<glm::mat4>& transforms) {
	glm::mat4 identity;

	const float ticksPerSecond = (float) (scene->mAnimations[0]->mTicksPerSecond != 0 ? scene->mAnimations[0]->mTicksPerSecond : 25.0f);
	const float timeInTicks = timeInSeconds * ticksPerSecond;
	const float animationTime = fmod(timeInTicks, (float) scene->mAnimations[0]->mDuration);

	readNodeHierarchy(scene, animationTime, scene->mRootNode, identity);

	transforms.resize(_numBones);

	for (uint32_t i = 0; i < _numBones; i++) {
		transforms[i] = _boneInfo[i].finalTransformation;
	}
}

const aiNodeAnim* Mesh::findNodeAnim(const aiAnimation* animation, const std::string& nodeName) {
	for (uint32_t i = 0; i < animation->mNumChannels; i++) {
		const aiNodeAnim* nodeAnim = animation->mChannels[i];
		if (!strcmp(nodeAnim->mNodeName.data, nodeName.c_str())) {
			return nodeAnim;
		}
	}

	return nullptr;
}

void Mesh::loadTextureImages(const aiScene* scene, const std::string& filename) {
	std::string::size_type slashIndex = filename.find_last_of("/");
	std::string dir;

	if (slashIndex == std::string::npos) {
		dir = ".";
	} else if (slashIndex == 0) {
		dir = "/";
	} else {
		dir = filename.substr(0, slashIndex);
	}

	_images.resize(scene->mNumMaterials);
	for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
		const aiMaterial* material = scene->mMaterials[i];
		const aiTextureType texType = aiTextureType_DIFFUSE;
		if (material->GetTextureCount(texType) <= 0) {
			Log::warn("No textures for texture type %i at index %i", texType, i);
			continue;
		}

		aiString path;
		if (material->GetTexture(texType, i, &path, nullptr, nullptr, nullptr, nullptr, nullptr) != AI_SUCCESS) {
			Log::warn("Could not get texture path for material index %i", i);
			continue;
		}

		std::string p(path.data);

		if (p.substr(0, 2) == ".\\") {
			p = p.substr(2, p.size() - 2);
		}

		const std::string fullPath = dir + "/" + p;
		_images[i] = image::loadImage(fullPath, false);
	}
}

int Mesh::render() {
	if (_state != io::IOSTATE_LOADED) {
		return 0;
	}
	glBindVertexArray(_vertexArrayObject);
	int drawCalls = 0;
	for (const GLMeshData& mesh : _meshData) {
		const uint32_t matIdx = mesh.materialIndex;
		if (matIdx < _textures.size() && _textures[matIdx]) {
			_textures[matIdx]->bind();
		}
		glDrawElementsBaseVertex(GL_TRIANGLES, mesh.noOfIndices, mesh.indexType, GL_OFFSET_CAST(sizeof(uint32_t) * mesh.baseIndex), mesh.baseVertex);
		++drawCalls;
	}
	GL_checkError();
	return drawCalls;
}

}
