/**
 * @file
 */

#include "Mesh.h"
#include "GLFunc.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/Log.h"

namespace video {

namespace {
const aiVector3D VECZERO(0.0f, 0.0f, 0.0f);
const aiColor4D COLOR_BLACK(0.0f, 0.0f, 0.0f, 0.0f);
}

Mesh::Mesh() :
		io::IOResource() {
}

Mesh::~Mesh() {
	core_assert_msg(_vertexArrayObject == 0u, "Mesh %s was not properly shut down", _filename.c_str());
	core_assert_msg(_vertexArrayObjectNormals == 0u, "Mesh %s was not properly shut down", _filename.c_str());
	shutdown();
}

void Mesh::shutdown() {
	_importer.FreeScene();
	_textures.clear();
	_images.clear();
	_meshData.clear();

	_vertices.clear();
	_indices.clear();
	_boneInfo.clear();
	_boneMapping.clear();
	_globalInverseTransform = glm::mat4();
	_numBones = 0;

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
	if (_vertexArrayObjectNormals != 0u) {
		glDeleteVertexArrays(1, &_vertexArrayObjectNormals);
		_vertexArrayObjectNormals = 0u;
	}
}

bool Mesh::loadMesh(const std::string& filename) {
#if 0
	// TODO: implement custom io handler to support meshes that are split over several files (like obj)
	class MeshIOSystem : public Assimp::IOSystem {
	};
	MeshIOSystem iosystem;
	_importer.SetIOHandler(&iosystem);
#endif
	_filename = filename;
	_scene = _importer.ReadFile(filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_FindDegenerates);
	if (_scene == nullptr || _scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE || !_scene->mRootNode) {
		Log::error("Error parsing '%s': '%s'\n", filename.c_str(), _importer.GetErrorString());
		_state = io::IOSTATE_FAILED;
		return false;
	}

	for (uint32_t i = 0; i < _scene->mNumAnimations; ++i) {
		const aiAnimation* animation = _scene->mAnimations[i];
		Log::debug("Animation %i: %s", i, animation->mName.C_Str());
	}

	_globalInverseTransform = glm::inverse(toMat4(_scene->mRootNode->mTransformation));

	_meshData.resize(_scene->mNumMeshes);

	uint32_t numIndices = 0u;
	uint32_t numVertices = 0u;

	for (uint32_t i = 0; i < _meshData.size(); ++i) {
		const aiMesh* mesh = _scene->mMeshes[i];
		GLMeshData& meshData = _meshData[i];
		meshData.materialIndex = mesh->mMaterialIndex;
		meshData.noOfIndices = mesh->mNumFaces * 3;
		meshData.baseVertex = numVertices;
		meshData.baseIndex = numIndices;
		meshData.indexType = GL_UNSIGNED_INT;

		numVertices += mesh->mNumVertices;
		numIndices += meshData.noOfIndices;
	}

	_vertices.reserve(numVertices);
	_indices.reserve(numIndices);
	_boneInfo.clear();

	for (uint32_t i = 0u; i < _meshData.size(); ++i) {
		const aiMesh* mesh = _scene->mMeshes[i];

		if (mesh->HasVertexColors(0)) {
			Log::debug("Mesh has vertex color");
		}

		for (uint32_t fi = 0; fi < mesh->mNumFaces; ++fi) {
			const aiFace& face = mesh->mFaces[fi];
			core_assert(face.mNumIndices == 3);
			_indices.push_back(face.mIndices[0]);
			_indices.push_back(face.mIndices[1]);
			_indices.push_back(face.mIndices[2]);
		}

		for (uint32_t vi = 0; vi < mesh->mNumVertices; ++vi) {
			const aiVector3D& pos = mesh->mVertices[vi];
			const aiVector3D& normal = mesh->mNormals[vi];
			const aiVector3D& texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][vi] : VECZERO;
			const aiColor4D& color = mesh->HasVertexColors(0) ? mesh->mColors[0][vi] : COLOR_BLACK;

			if (pos.x < _aabbMins.x) {
				_aabbMins.x = pos.x;
			}
			if (pos.x > _aabbMaxs.x) {
				_aabbMaxs.x = pos.x;
			}
			if (pos.y < _aabbMins.y) {
				_aabbMins.y = pos.y;
			}
			if (pos.y > _aabbMaxs.y) {
				_aabbMaxs.y = pos.y;
			}
			if (pos.z < _aabbMins.z) {
				_aabbMins.z = pos.z;
			}
			if (pos.z > _aabbMaxs.z) {
				_aabbMaxs.z = pos.z;
			}

			_vertices.push_back(MeshVertex(pos, normal, texCoord, color));
		}
		loadBones(i, mesh);
	}

	loadTextureImages(_scene, filename);
	_readyToInit = true;
	Log::info("Loaded mesh %s with %i vertices and %i indices", filename.c_str(), (int)_vertices.size(), (int)_indices.size());
	return true;
}

bool Mesh::initMesh(Shader& shader, float timeInSeconds, uint8_t animationIndex) {
	if (_state != io::IOSTATE_LOADED) {
		if (!_readyToInit) {
			return false;
		}

		for (const image::ImagePtr& i : _images) {
			if (i && i->isLoading()) {
				return false;
			}
		}

		glGenVertexArrays(1, &_vertexArrayObjectNormals);
		glGenBuffers(1, &_vboNormals);

		glGenVertexArrays(1, &_vertexArrayObject);
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

		glBindVertexArray(_vertexArrayObject);

		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(MeshVertex) * _vertices.size(), &_vertices[0], GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(_indices[0]) * _indices.size(), &_indices[0], GL_STATIC_DRAW);

		glBindVertexArray(0);

		GLdouble buf[2];
		glGetDoublev(GL_SMOOTH_LINE_WIDTH_RANGE, buf);
		_lineWidth = std::min((float)buf[1], _lineWidth);
	}

	_timeInSeconds = timeInSeconds;
	_animationIndex = animationIndex;

	if (&shader != _lastShader) {
		core_assert(shader.isActive());
		_lastShader = &shader;
		glBindVertexArray(_vertexArrayObject);
		glBindBuffer(GL_ARRAY_BUFFER, _vbo);

#define enable(attrib) \
		if (shader.hasAttribute("a"#attrib)) { \
			const int loc = shader.enableVertexAttributeArray("a"#attrib); \
			const int components = sizeof(MeshVertex::attrib) / sizeof(decltype(MeshVertex::attrib)::value_type); \
			shader.setVertexAttribute(loc, components, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), GL_OFFSET_CAST(offsetof(MeshVertex, attrib))); \
		}

		enable(_pos);
		enable(_texcoords);
		enable(_color);
		enable(_norm);

#undef enable

		if (shader.hasAttribute("a_boneids")) {
			const int loc = shader.enableVertexAttributeArray("a_boneids");
			shader.setVertexAttributeInt(loc, NUM_BONES_PER_VEREX, GL_INT, sizeof(MeshVertex), GL_OFFSET_CAST(offsetof(MeshVertex, _boneIds)));
		}
		if (shader.hasAttribute("a_boneweights")) {
			const int loc = shader.enableVertexAttributeArray("a_boneweights");
			shader.setVertexAttribute(loc, NUM_BONES_PER_VEREX, GL_FLOAT, GL_FALSE, sizeof(MeshVertex), GL_OFFSET_CAST(offsetof(MeshVertex, _boneWeights)));
		}
		glBindVertexArray(0);
	}

	const int size = shader.getUniformArraySize("u_bonetransforms");
	if (size > 0) {
		core_assert(size == 100);
		glm::mat4 transforms[100];
		boneTransform(_timeInSeconds, &transforms[0], size, _animationIndex);
		shader.setUniformMatrixv("u_bonetransforms", &transforms[0], size);
	}

	return true;
}

void Mesh::loadBones(uint32_t meshIndex, const aiMesh* mesh) {
	Log::debug("Load %i bones", mesh->mNumBones);
	for (uint32_t i = 0u; i < mesh->mNumBones; ++i) {
		uint32_t boneIndex = 0u;
		const aiBone* aiBone = mesh->mBones[i];
		const std::string boneName(aiBone->mName.data);

		auto iter = _boneMapping.find(boneName);
		if (iter == _boneMapping.end()) {
			// Allocate an index for a new bone
			boneIndex = _numBones;
			++_numBones;
			const BoneInfo bi = { toMat4(aiBone->mOffsetMatrix), glm::mat4() };
			_boneInfo.push_back(bi);
			_boneMapping[boneName] = boneIndex;
		} else {
			boneIndex = iter->second;
		}

		Log::debug("Load bone %s with %i weights defined", boneName.c_str(), aiBone->mNumWeights);
		for (uint32_t j = 0u; j < aiBone->mNumWeights; ++j) {
			const aiVertexWeight& weights = aiBone->mWeights[j];
			const uint32_t vertexID = _meshData[meshIndex].baseVertex + weights.mVertexId;
			const float weight = weights.mWeight;
			_vertices[vertexID].addBoneData(boneIndex, weight);
		}
	}
}

uint32_t Mesh::findPosition(float animationTime, const aiNodeAnim* nodeAnim) {
	core_assert(nodeAnim->mNumPositionKeys > 0);
	for (uint32_t i = 0u; i < nodeAnim->mNumPositionKeys - 1; ++i) {
		if (animationTime < (float) nodeAnim->mPositionKeys[i + 1].mTime) {
			return i;
		}
	}

	core_assert_msg(false, "could not find a suitable position for animationTime %f", animationTime);

	return 0u;
}

uint32_t Mesh::findRotation(float animationTime, const aiNodeAnim* nodeAnim) {
	core_assert(nodeAnim->mNumRotationKeys > 0);
	for (uint32_t i = 0u; i < nodeAnim->mNumRotationKeys - 1; ++i) {
		if (animationTime < (float) nodeAnim->mRotationKeys[i + 1].mTime) {
			return i;
		}
	}

	core_assert_msg(false, "could not find a suitable rotation for animationTime %f", animationTime);

	return 0u;
}

uint32_t Mesh::findScaling(float animationTime, const aiNodeAnim* nodeAnim) {
	core_assert(nodeAnim->mNumScalingKeys > 0);
	for (uint32_t i = 0u; i < nodeAnim->mNumScalingKeys - 1; ++i) {
		if (animationTime < (float) nodeAnim->mScalingKeys[i + 1].mTime) {
			return i;
		}
	}

	core_assert_msg(false, "could not find a suitable scaling for animationTime %f", animationTime);

	return 0u;
}

glm::vec3 Mesh::calcInterpolatedPosition(float animationTime, const aiNodeAnim* nodeAnim) {
	if (nodeAnim->mNumPositionKeys == 1) {
		return toVec3(nodeAnim->mPositionKeys[0].mValue);
	}

	const uint32_t positionIndex = findPosition(animationTime, nodeAnim);
	const uint32_t nextPositionIndex = (positionIndex + 1);
	core_assert(nextPositionIndex < nodeAnim->mNumPositionKeys);
	const float deltaTime = (float) (nodeAnim->mPositionKeys[nextPositionIndex].mTime - nodeAnim->mPositionKeys[positionIndex].mTime);
	const float factor = (animationTime - (float) nodeAnim->mPositionKeys[positionIndex].mTime) / deltaTime;
	core_assert(factor >= 0.0f && factor <= 1.0f);
	const glm::vec3& start = toVec3(nodeAnim->mPositionKeys[positionIndex].mValue);
	const glm::vec3& end = toVec3(nodeAnim->mPositionKeys[nextPositionIndex].mValue);
	const glm::vec3& delta = end - start;
	return start + factor * delta;
}

glm::mat4 Mesh::calcInterpolatedRotation(float animationTime, const aiNodeAnim* nodeAnim) {
	// we need at least two values to interpolate...
	if (nodeAnim->mNumRotationKeys == 1) {
		return glm::mat4_cast(toQuat(nodeAnim->mRotationKeys[0].mValue));
	}

	const uint32_t rotationIndex = findRotation(animationTime, nodeAnim);
	const uint32_t nextRotationIndex = (rotationIndex + 1);
	core_assert(nextRotationIndex < nodeAnim->mNumRotationKeys);
	const float deltaTime = (float) (nodeAnim->mRotationKeys[nextRotationIndex].mTime - nodeAnim->mRotationKeys[rotationIndex].mTime);
	const float factor = (animationTime - (float) nodeAnim->mRotationKeys[rotationIndex].mTime) / deltaTime;
	core_assert(factor >= 0.0f && factor <= 1.0f);
	const glm::quat& startRotationQ = toQuat(nodeAnim->mRotationKeys[rotationIndex].mValue);
	const glm::quat& endRotationQ = toQuat(nodeAnim->mRotationKeys[nextRotationIndex].mValue);
	const glm::quat& out = glm::normalize(glm::slerp(startRotationQ, endRotationQ, factor));
	return glm::mat4_cast(out);
}

glm::vec3 Mesh::calcInterpolatedScaling(float animationTime, const aiNodeAnim* nodeAnim) {
	if (nodeAnim->mNumScalingKeys == 1) {
		return toVec3(nodeAnim->mScalingKeys[0].mValue);
	}

	const uint32_t scalingIndex = findScaling(animationTime, nodeAnim);
	const uint32_t nextScalingIndex = (scalingIndex + 1);
	core_assert(nextScalingIndex < nodeAnim->mNumScalingKeys);
	const float deltaTime = (float) (nodeAnim->mScalingKeys[nextScalingIndex].mTime - nodeAnim->mScalingKeys[scalingIndex].mTime);
	const float factor = (animationTime - (float) nodeAnim->mScalingKeys[scalingIndex].mTime) / deltaTime;
	core_assert(factor >= 0.0f && factor <= 1.0f);
	const glm::vec3& start = toVec3(nodeAnim->mScalingKeys[scalingIndex].mValue);
	const glm::vec3& end = toVec3(nodeAnim->mScalingKeys[nextScalingIndex].mValue);
	const glm::vec3 delta = end - start;
	return start + factor * delta;
}

void Mesh::readNodeHierarchy(const aiAnimation* animation, float animationTime, const aiNode* node, const glm::mat4& parentTransform) {
	const std::string nodeName(node->mName.data);
	glm::mat4 nodeTransformation;
	const aiNodeAnim* nodeAnim = findNodeAnim(animation, nodeName);

	if (nodeAnim != nullptr) {
		// Interpolate scaling and generate scaling transformation matrix
		const glm::vec3& scaling = calcInterpolatedScaling(animationTime, nodeAnim);
		const glm::mat4& scalingM = glm::scale(glm::mat4(), glm::vec3(scaling.x, scaling.y, scaling.z));

		// Interpolate rotation and generate rotation transformation matrix
		const glm::mat4& rotationM = calcInterpolatedRotation(animationTime, nodeAnim);

		// Interpolate translation and generate translation transformation matrix
		const glm::vec3& translation = calcInterpolatedPosition(animationTime, nodeAnim);
		const glm::mat4& translationM = glm::translate(glm::mat4(), glm::vec3(translation.x, translation.y, translation.z));

		// Combine the above transformations
		nodeTransformation = translationM * rotationM * scalingM;
	} else {
		nodeTransformation = toMat4(node->mTransformation);
	}

	const glm::mat4 globalTransformation = parentTransform * nodeTransformation;

	auto iter = _boneMapping.find(nodeName);
	if (iter != _boneMapping.end()) {
		const uint32_t boneIndex = iter->second;
		_boneInfo[boneIndex].finalTransformation = _globalInverseTransform * globalTransformation * _boneInfo[boneIndex].boneOffset;
		Log::trace("update bone transform for node name %s (index: %u)", nodeName.c_str(), boneIndex);
	} else {
		Log::trace("Could not find bone mapping for node name %s", nodeName.c_str());
	}

	for (uint32_t i = 0u; i < node->mNumChildren; ++i) {
		readNodeHierarchy(animation, animationTime, node->mChildren[i], globalTransformation);
	}
}

void Mesh::boneTransform(float timeInSeconds, glm::mat4* transforms, size_t size, uint8_t animationIndex) {
	const glm::mat4 identity;

	if (_numBones == 0 || _scene->mNumAnimations == 0) {
		core_assert(size >= 1);
		transforms[0] = identity;
		return;
	}
	core_assert(_numBones <= size);

	const aiAnimation* animation = _scene->mAnimations[animationIndex];
	const float ticksPerSecond = (float) (animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f);
	const float timeInTicks = timeInSeconds * ticksPerSecond;
	const float animationTime = fmod(timeInTicks, (float) animation->mDuration);

	readNodeHierarchy(animation, animationTime, _scene->mRootNode, identity);

	for (uint32_t i = 0u; i < _numBones; ++i) {
		transforms[i] = _boneInfo[i].finalTransformation;
	}
}

const aiNodeAnim* Mesh::findNodeAnim(const aiAnimation* animation, const std::string& nodeName) {
	for (uint32_t i = 0u; i < animation->mNumChannels; ++i) {
		const aiNodeAnim* nodeAnim = animation->mChannels[i];
		if (!strcmp(nodeAnim->mNodeName.data, nodeName.c_str())) {
			return nodeAnim;
		}
	}

	Log::trace("Could not find animation node for %s", nodeName.c_str());
	return nullptr;
}

void Mesh::loadTextureImages(const aiScene* scene, const std::string& filename) {
	std::string::size_type slashIndex = filename.find_last_of('/');
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
			Log::debug("No textures for texture type %i at index %i", texType, i);
			continue;
		}

		aiString path;
		if (material->GetTexture(texType, 0, &path) != AI_SUCCESS) {
			Log::warn("Could not get texture path for material index %i", i);
			continue;
		}
		Log::debug("Texture for texture type %i at index %i: %s", texType, i, path.data);

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
		mesh.draw();
		++drawCalls;
	}
	// Make sure the VAO is not changed from the outside
	glBindVertexArray(0);
	GL_checkError();

	return drawCalls;
}

int Mesh::renderNormals(video::Shader& shader) {
	core_assert(shader.isActive());
	struct MeshNormals {
		struct AttributeData {
			glm::vec4 vertex;
			glm::vec3 color;
		};
		std::vector<AttributeData> data;

		inline size_t size() const {
			return sizeof(AttributeData) * data.size();
		}

		inline void reserve(size_t amount) {
			data.reserve(amount);
		}
	};

	if (_vertexArrayObjectNormals == 0u) {
		return 0;
	}

	MeshNormals normalData;
	normalData.reserve(_vertices.size() * 2);
	for (const core::Vertex& v : _vertices) {
		glm::mat4 bonetrans;
		for (int i = 0; i < NUM_BONES_PER_VEREX; ++i) {
			const glm::mat4& bmat = _boneInfo[v._boneIds[i]].finalTransformation * v._boneWeights[i];
			bonetrans += bmat;
		}
		const glm::vec4& pos = bonetrans * glm::vec4(v._pos, 1.0f);
		const glm::vec4& norm = bonetrans * glm::vec4(v._norm, 0.0f);
		const glm::vec4& extended = pos + 2.0f * norm;
		normalData.data.push_back(MeshNormals::AttributeData{ pos,      glm::vec3(core::Color::Red)    });
		normalData.data.push_back(MeshNormals::AttributeData{ extended, glm::vec3(core::Color::Yellow) });
	}

	glBindVertexArray(_vertexArrayObjectNormals);
	glBindBuffer(GL_ARRAY_BUFFER, _vboNormals);
	glBufferData(GL_ARRAY_BUFFER, normalData.size(), &normalData.data[0], GL_STATIC_DRAW);

	if (shader.hasAttribute("a_pos")) {
		const int loc = shader.enableVertexAttributeArray("a_pos");
		core_assert(loc >= 0);
		const int components = sizeof(MeshNormals::AttributeData::vertex) / sizeof(decltype(MeshNormals::AttributeData::vertex)::value_type);
		shader.setVertexAttribute(loc, components, GL_FLOAT, GL_FALSE, sizeof(MeshNormals::AttributeData), GL_OFFSET_CAST(offsetof(MeshNormals::AttributeData, vertex)));
	}
	if (shader.hasAttribute("a_color")) {
		const int loc = shader.enableVertexAttributeArray("a_color");
		core_assert(loc >= 0);
		const int components = sizeof(MeshNormals::AttributeData::color) / sizeof(decltype(MeshNormals::AttributeData::color)::value_type);
		shader.setVertexAttribute(loc, components, GL_FLOAT, GL_FALSE, sizeof(MeshNormals::AttributeData), GL_OFFSET_CAST(offsetof(MeshNormals::AttributeData, color)));
	}

	glLineWidth(_lineWidth);
	glDrawArrays(GL_LINES, 0, normalData.data.size());
	glLineWidth(1.0f);

	glBindVertexArray(0);

	return 1;
}

}
