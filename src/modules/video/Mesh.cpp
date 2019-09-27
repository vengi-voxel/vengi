/**
 * @file
 */

#include "Mesh.h"
#include "Renderer.h"
#include "core/Common.h"
#include "core/Array.h"
#include "core/App.h"
#include "io/Filesystem.h"
#include "core/Log.h"
#include "core/GLM.h"
#include "video/ScopedLineWidth.h"
#include "video/Types.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace video {

namespace {
const aiVector3D VECZERO(0.0f, 0.0f, 0.0f);
const aiColor4D COLOR_BLACK(0.0f, 0.0f, 0.0f, 1.0f);

static inline glm::vec3 convert(const aiVector3D& vector) {
	return glm::vec3(vector.x, vector.y, vector.z);
}

static inline glm::quat convert(const aiQuaternion& quat) {
	return glm::quat(quat.w, quat.x, quat.y, quat.z);
}

static inline glm::mat4 convert(const aiMatrix4x4& m) {
	glm::mat4 mat;
	mat[0][0] = m.a1;
	mat[1][0] = m.a2;
	mat[2][0] = m.a3;
	mat[3][0] = m.a4;
	mat[0][1] = m.b1;
	mat[1][1] = m.b2;
	mat[2][1] = m.b3;
	mat[3][1] = m.b4;
	mat[0][2] = m.c1;
	mat[1][2] = m.c2;
	mat[2][2] = m.c3;
	mat[3][2] = m.c4;
	mat[0][3] = m.d1;
	mat[1][3] = m.d2;
	mat[2][3] = m.d3;
	mat[3][3] = m.d4;
	return mat;
}

static inline core::Vertex convertVertex(const aiVector3D& p, const aiVector3D& n, const aiVector3D& t, const aiColor4D& c) {
	return core::Vertex(convert(p), convert(n), glm::vec2(t.x, t.y), glm::vec4(c.r, c.g, c.b, c.a));
}

}

Mesh::Mesh() :
		io::IOResource(), _importer(new Assimp::Importer()) {
}

Mesh::~Mesh() {
	shutdown();
	delete _importer;
}

void Mesh::shutdown() {
	_importer->FreeScene();
	_textures.clear();
	_images.clear();
	_meshData.clear();
	_vertexBuffer.shutdown();
	_vertexBufferLines.shutdown();
	_vertexBufferLinesIndex = -1;
	_vertexBufferIndex = -1;

	_vertices.clear();
	_indices.clear();
	_boneInfo.clear();
	_boneMapping.clear();
	_globalInverseTransform = glm::mat4(1.0f);
	_numBones = 0;

	_readyToInit = false;
}

bool Mesh::loadMesh(const std::string& filename) {
	if (filename.empty()) {
		Log::error("Failed to load mesh: No filename given");
		return false;
	}
	const io::FilePtr& f = core::App::getInstance()->filesystem()->open(filename, io::FileMode::Read);
	if (!f->exists()) {
		Log::error("Could not open mesh %s", filename.c_str());
		return false;
	}
	_filename = f->name();
	_scene = _importer->ReadFile(_filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_FindDegenerates);
	if (_scene == nullptr) {
		_state = io::IOSTATE_FAILED;
		Log::error("Error parsing '%s': '%s'", _filename.c_str(), _importer->GetErrorString());
		return false;
	}
	if (_scene->mRootNode == nullptr) {
		Log::error("Scene doesn't have a root node'%s': '%s'", _filename.c_str(), _importer->GetErrorString());
		return false;
	}
	Log::info("Animations found %i", (int)_scene->mNumAnimations);
	for (uint32_t i = 0; i < _scene->mNumAnimations; ++i) {
		const aiAnimation* animation = _scene->mAnimations[i];
		Log::info("Animation %i: %s", i, animation->mName.C_Str());
	}
	if (_scene->mFlags == AI_SCENE_FLAGS_INCOMPLETE) {
		Log::warn("Scene incomplete '%s': '%s'", filename.c_str(), _importer->GetErrorString());
	}

	_globalInverseTransform = glm::inverse(convert(_scene->mRootNode->mTransformation));

	_meshData.resize(_scene->mNumMeshes);

	uint32_t numIndices = 0u;
	uint32_t numVertices = 0u;

	for (uint32_t i = 0; i < _meshData.size(); ++i) {
		const aiMesh* mesh = _scene->mMeshes[i];
		RenderMeshData& meshData = _meshData[i];
		meshData.materialIndex = mesh->mMaterialIndex;
		meshData.noOfIndices = mesh->mNumFaces * 3;
		meshData.baseVertex = numVertices;
		meshData.baseIndex = numIndices;

		numVertices += mesh->mNumVertices;
		numIndices += meshData.noOfIndices;
	}

	_vertices.reserve(numVertices);
	_indices.reserve(numIndices);
	_boneInfo.clear();

	_aabbMins = glm::vec3((std::numeric_limits<float>::max)());
	_aabbMaxs = glm::vec3((std::numeric_limits<float>::min)());

	for (uint32_t i = 0u; i < _meshData.size(); ++i) {
		const aiMesh* mesh = _scene->mMeshes[i];

		for (uint32_t fi = 0; fi < mesh->mNumFaces; ++fi) {
			const aiFace& face = mesh->mFaces[fi];
			core_assert(face.mNumIndices == 3);
			_indices.push_back(face.mIndices[0]);
			_indices.push_back(face.mIndices[1]);
			_indices.push_back(face.mIndices[2]);
		}

		for (uint32_t vi = 0; vi < mesh->mNumVertices; ++vi) {
			const aiVector3D& pos = mesh->mVertices[vi];
			const aiVector3D& normal = mesh->HasNormals() ? mesh->mNormals[vi] : VECZERO;
			const aiVector3D& texCoord = mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0][vi] : VECZERO;
			const aiColor4D& color = mesh->HasVertexColors(0) ? mesh->mColors[0][vi] : COLOR_BLACK;

			if (pos.x <= _aabbMins.x) {
				_aabbMins.x = pos.x;
			}
			if (pos.x > _aabbMaxs.x) {
				_aabbMaxs.x = pos.x;
			}
			if (pos.y <= _aabbMins.y) {
				_aabbMins.y = pos.y;
			}
			if (pos.y > _aabbMaxs.y) {
				_aabbMaxs.y = pos.y;
			}
			if (pos.z <= _aabbMins.z) {
				_aabbMins.z = pos.z;
			}
			if (pos.z > _aabbMaxs.z) {
				_aabbMaxs.z = pos.z;
			}

			_vertices.emplace_back(convertVertex(pos, normal, texCoord, color));
		}
		loadBones(i, mesh);
	}

	const std::string basename(core::string::stripExtension(filename.c_str()));
	const std::string::size_type slashIndex = filename.find_last_of('/');
	std::string dir;

	if (slashIndex == std::string::npos) {
		dir = ".";
	} else if (slashIndex == 0) {
		dir = "/";
	} else {
		dir = filename.substr(0, slashIndex);
	}

	loadTextureImages(_scene, dir, basename);
	_readyToInit = true;
	Log::info("Loaded mesh %s with %i vertices and %i indices", filename.c_str(), (int)_vertices.size(), (int)_indices.size());
	return true;
}

void Mesh::setupBufferAttributes(Shader& shader) {
	_vertexBuffer.clearAttributes();

	const int posLocation = shader.checkAttributeLocation("a_pos");
	if (posLocation != -1) {
		video::Attribute attribPos;
		attribPos.bufferIndex = _vertexBufferIndex;
		attribPos.location = posLocation;
		attribPos.stride = sizeof(Vertices::value_type);
		attribPos.size = shader.getAttributeComponents(attribPos.location);
		attribPos.type = video::mapType<decltype(Vertices::value_type::_pos)::value_type>();
		attribPos.offset = offsetof(Vertices::value_type, _pos);
		core_assert_always(_vertexBuffer.addAttribute(attribPos));
	}

	const int texcoordsLocation = shader.checkAttributeLocation("a_texcoords");
	if (texcoordsLocation != -1) {
		video::Attribute attribTexCoord;
		attribTexCoord.bufferIndex = _vertexBufferIndex;
		attribTexCoord.location = texcoordsLocation;
		attribTexCoord.stride = sizeof(Vertices::value_type);
		attribTexCoord.size = shader.getAttributeComponents(attribTexCoord.location);
		attribTexCoord.type = video::mapType<decltype(Vertices::value_type::_texcoords)::value_type>();
		attribTexCoord.offset = offsetof(Vertices::value_type, _texcoords);
		core_assert_always(_vertexBuffer.addAttribute(attribTexCoord));
	}

	const int colorLocation = shader.checkAttributeLocation("a_color");
	if (colorLocation != -1) {
		video::Attribute attribColor;
		attribColor.bufferIndex = _vertexBufferIndex;
		attribColor.location = colorLocation;
		attribColor.stride = sizeof(Vertices::value_type);
		attribColor.size = shader.getAttributeComponents(attribColor.location);
		attribColor.type = video::mapType<decltype(Vertices::value_type::_color)::value_type>();
		attribColor.offset = offsetof(Vertices::value_type, _color);
		core_assert_always(_vertexBuffer.addAttribute(attribColor));
	}

	const int normLocation = shader.checkAttributeLocation("a_norm");
	if (normLocation != -1) {
		video::Attribute attribNorm;
		attribNorm.bufferIndex = _vertexBufferIndex;
		attribNorm.location = normLocation;
		attribNorm.stride = sizeof(Vertices::value_type);
		attribNorm.size = shader.getAttributeComponents(attribNorm.location);
		attribNorm.type = video::mapType<decltype(Vertices::value_type::_norm)::value_type>();
		attribNorm.offset = offsetof(Vertices::value_type, _norm);
		core_assert_always(_vertexBuffer.addAttribute(attribNorm));
	}

	const int boneIdsLocation = shader.checkAttributeLocation("a_boneids");
	if (boneIdsLocation != -1) {
		video::Attribute attribBoneIds;
		attribBoneIds.bufferIndex = _vertexBufferIndex;
		attribBoneIds.location = boneIdsLocation;
		attribBoneIds.stride = sizeof(Vertices::value_type);
		attribBoneIds.size = shader.getAttributeComponents(attribBoneIds.location);
		attribBoneIds.type = video::mapType<decltype(Vertices::value_type::_boneIds[0])>();
		attribBoneIds.offset = offsetof(Vertices::value_type, _boneIds);
		attribBoneIds.typeIsInt = true;
		core_assert_always(_vertexBuffer.addAttribute(attribBoneIds));
	}

	const int boneweightsLocation = shader.checkAttributeLocation("a_boneweights");
	if (boneweightsLocation != -1) {
		video::Attribute attribBoneWeights;
		attribBoneWeights.bufferIndex = _vertexBufferIndex;
		attribBoneWeights.location = boneweightsLocation;
		attribBoneWeights.stride = sizeof(Vertices::value_type);
		attribBoneWeights.size = shader.getAttributeComponents(attribBoneWeights.location);
		attribBoneWeights.type = video::mapType<decltype(Vertices::value_type::_boneWeights[0])>();
		attribBoneWeights.offset = offsetof(Vertices::value_type, _boneWeights);
		core_assert_always(_vertexBuffer.addAttribute(attribBoneWeights));
	}
}

void Mesh::setupLineBufferAttributes(Shader& shader) {
	if (_vertexBufferLines.attributes() == 2) {
		return;
	}
	_vertexBufferLines.clearAttributes();

	video::Attribute attribPos;
	attribPos.bufferIndex = _vertexBufferLinesIndex;
	attribPos.location = shader.enableVertexAttributeArray("a_pos");
	attribPos.stride = sizeof(MeshLines::AttributeData);
	attribPos.size = shader.getAttributeComponents(attribPos.location);
	attribPos.type = video::mapType<decltype(MeshLines::AttributeData::vertex)::value_type>();
	attribPos.offset = offsetof(MeshLines::AttributeData, vertex);
	_vertexBufferLines.addAttribute(attribPos);

	video::Attribute attribColor;
	attribColor.bufferIndex = _vertexBufferLinesIndex;
	attribColor.location = shader.enableVertexAttributeArray("a_color");
	attribColor.stride = sizeof(MeshLines::AttributeData);
	attribColor.size = shader.getAttributeComponents(attribColor.location);
	attribColor.type = video::mapType<decltype(MeshLines::AttributeData::color)::value_type>();
	attribColor.offset = offsetof(MeshLines::AttributeData, color);
	_vertexBufferLines.addAttribute(attribColor);
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

		_textures.clear();
		_textures.resize(_images.size());
		int materialIndex = 0;
		for (const image::ImagePtr& i : _images) {
			if (i && i->isLoaded()) {
				_textures[materialIndex++] = createTextureFromImage(i);
			} else {
				++materialIndex;
			}
		}
		if (materialIndex == 0) {
			_textures.push_back(createWhiteTexture("***empty***"));
		}
		_images.clear();

		_state = io::IOSTATE_LOADED;

		_vertexBufferLinesIndex = _vertexBufferLines.create();
		_vertexBufferLines.setMode(_vertexBufferLinesIndex, BufferMode::Dynamic);

		_vertexBufferIndex = _vertexBuffer.create(_vertices);
		_vertexBuffer.create(_indices, BufferType::IndexBuffer);
	}

	_timeInSeconds = timeInSeconds;
	_animationIndex = animationIndex;
	if (_animationIndex >= _scene->mNumAnimations) {
		_animationIndex = 0l;
	}

	if (&shader != _lastShader) {
		core_assert(shader.isActive());
		_lastShader = &shader;
		setupBufferAttributes(shader);
	}

	if (shader.hasUniform("u_vertexskinning")) {
		shader.setUniformi("u_vertexskinning", _numBones);
	}

	const int size = shader.getUniformArraySize("u_bonetransforms");
	if (size > 0) {
		core_assert_always(size == 100);
		glm::mat4 transforms[100];
		// TODO: size _numBones?
		// TODO: use uniform block
		boneTransform(transforms, size);
		shader.setUniformMatrixv("u_bonetransforms", transforms, size);
	}

	return true;
}

void Mesh::loadBones(uint32_t meshIndex, const aiMesh* mesh) {
	if (!mesh->HasBones()) {
		return;
	}
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
			const BoneInfo bi { convert(aiBone->mOffsetMatrix), glm::mat4(1.0f) };
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

static glm::vec3 interpolateStep(const aiVectorKey& currentStep, const aiVectorKey& nextStep, float animationTime) {
	const float deltaTime = (float) (nextStep.mTime - currentStep.mTime);
	const float factor = (animationTime - (float) currentStep.mTime) / deltaTime;
	core_assert_msg(factor >= -1.0f && factor <= 1.0f, "Factor is not clamped: %f", factor);
	const glm::vec3& start = convert(currentStep.mValue);
	const glm::vec3& end = convert(nextStep.mValue);
	return glm::lerp(start, end, factor);
}

glm::vec3 Mesh::calcInterpolatedPosition(float animationTime, const aiNodeAnim* nodeAnim) {
	if (nodeAnim->mNumPositionKeys == 1) {
		return convert(nodeAnim->mPositionKeys[0].mValue);
	}

	const uint32_t positionIndex = findPosition(animationTime, nodeAnim);
	const uint32_t nextPositionIndex = (positionIndex + 1);
	core_assert(nextPositionIndex < nodeAnim->mNumPositionKeys);
	return interpolateStep(nodeAnim->mPositionKeys[positionIndex], nodeAnim->mPositionKeys[nextPositionIndex], animationTime);
}

glm::mat4 Mesh::calcInterpolatedRotation(float animationTime, const aiNodeAnim* nodeAnim) {
	// we need at least two values to interpolate...
	if (nodeAnim->mNumRotationKeys == 1) {
		return glm::mat4_cast(convert(nodeAnim->mRotationKeys[0].mValue));
	}

	const uint32_t rotationIndex = findRotation(animationTime, nodeAnim);
	const uint32_t nextRotationIndex = (rotationIndex + 1);
	core_assert(nextRotationIndex < nodeAnim->mNumRotationKeys);
	const float deltaTime = (float) (nodeAnim->mRotationKeys[nextRotationIndex].mTime - nodeAnim->mRotationKeys[rotationIndex].mTime);
	const float factor = (animationTime - (float) nodeAnim->mRotationKeys[rotationIndex].mTime) / deltaTime;
	core_assert_msg(factor >= -1.0f && factor <= 1.0f, "Factor is not clamped: %f", factor);
	const glm::quat& startRotationQ = convert(nodeAnim->mRotationKeys[rotationIndex].mValue);
	const glm::quat& endRotationQ = convert(nodeAnim->mRotationKeys[nextRotationIndex].mValue);
	const glm::quat& out = glm::normalize(glm::slerp(startRotationQ, endRotationQ, factor));
	return glm::mat4_cast(out);
}

glm::vec3 Mesh::calcInterpolatedScaling(float animationTime, const aiNodeAnim* nodeAnim) {
	if (nodeAnim->mNumScalingKeys == 1) {
		return convert(nodeAnim->mScalingKeys[0].mValue);
	}

	const uint32_t scalingIndex = findScaling(animationTime, nodeAnim);
	const uint32_t nextScalingIndex = (scalingIndex + 1);
	core_assert(nextScalingIndex < nodeAnim->mNumScalingKeys);
	return interpolateStep(nodeAnim->mScalingKeys[scalingIndex], nodeAnim->mScalingKeys[nextScalingIndex], animationTime);
}

void Mesh::readNodeHierarchy(const aiAnimation* animation, float animationTime, const aiNode* node, const glm::mat4& parentTransform) {
	const std::string nodeName(node->mName.data);
	glm::mat4 nodeTransformation;
	const aiNodeAnim* nodeAnim = findNodeAnim(animation, nodeName);

	if (nodeAnim != nullptr) {
		// Interpolate scaling and generate scaling transformation matrix
		const glm::vec3& scaling = calcInterpolatedScaling(animationTime, nodeAnim);
		const glm::mat4& scalingM = glm::scale(scaling);

		// Interpolate rotation and generate rotation transformation matrix
		const glm::mat4& rotationM = calcInterpolatedRotation(animationTime, nodeAnim);

		// Interpolate translation and generate translation transformation matrix
		const glm::vec3& translation = calcInterpolatedPosition(animationTime, nodeAnim);
		const glm::mat4& translationM = glm::translate(translation);

		// Combine the above transformations
		nodeTransformation = translationM * rotationM * scalingM;
	} else {
		nodeTransformation = convert(node->mTransformation);
	}

	const glm::mat4 globalTransformation = parentTransform * nodeTransformation;

	auto iter = _boneMapping.find(nodeName);
	if (iter != _boneMapping.end()) {
		const uint32_t boneIndex = iter->second;
		// https://stackoverflow.com/questions/29184311/how-to-rotate-a-skinned-models-bones-in-c-using-assimp
		_boneInfo[boneIndex].finalTransformation = _globalInverseTransform * globalTransformation * _boneInfo[boneIndex].boneOffset;
		Log::trace("update bone transform for node name %s (index: %u)", nodeName.c_str(), boneIndex);
	} else {
		Log::trace("Could not find bone mapping for node name %s", nodeName.c_str());
	}

	for (uint32_t i = 0u; i < node->mNumChildren; ++i) {
		readNodeHierarchy(animation, animationTime, node->mChildren[i], globalTransformation);
	}
}

void Mesh::boneTransform(glm::mat4* transforms, size_t size) {
	if (_numBones == 0 || _scene->mNumAnimations == 0) {
		core_assert_always(size >= 1);
		transforms[0] = glm::mat4(1.0f);
		return;
	}
	core_assert_always(_animationIndex < _scene->mNumAnimations);
	core_assert_always(_numBones <= size);

	const aiAnimation* animation = _scene->mAnimations[_animationIndex];
	const float ticksPerSecond = (float) (animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f);
	const float timeInTicks = _timeInSeconds * ticksPerSecond;
	const float animationTime = fmod(timeInTicks, (float) animation->mDuration);

	readNodeHierarchy(animation, animationTime, _scene->mRootNode, glm::mat4(1.0f));

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

void Mesh::loadTextureImages(const aiScene* scene, const std::string& dir, const std::string& basename) {
	_images.resize(scene->mNumMaterials);
	for (uint32_t i = 0; i < scene->mNumMaterials; i++) {
		const aiMaterial* material = scene->mMaterials[i];
		const aiTextureType texType = aiTextureType_DIFFUSE;
		const int textureCount = material->GetTextureCount(texType);
		if (textureCount <= 0) {
			Log::debug("No textures for texture type %i at index %i", texType, i);
			continue;
		}

		for (int n = 0; n < textureCount; ++n) {
			aiString path;
			if (material->GetTexture(texType, i, &path) != AI_SUCCESS) {
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
			if (_images[i]->isLoaded()) {
				break;
			}
		}
		if (!_images[i]->isLoaded()) {
			// as a fallback try to load a texture in the same dir as the model with the same base naem
			_images[i] = image::loadImage(basename + ".png", false);
		}
	}
}

int Mesh::render() {
	if (_state != io::IOSTATE_LOADED) {
		return 0;
	}
	video::ScopedBuffer scopedBuf(_vertexBuffer);
	int drawCalls = 0;
	for (const RenderMeshData& mesh : _meshData) {
		const uint32_t matIdx = mesh.materialIndex;
		if (matIdx < _textures.size() && _textures[matIdx]) {
			_textures[matIdx]->bind(video::TextureUnit::Zero);
		}
		video::drawElementsBaseVertex<Indices::value_type>(video::Primitive::Triangles, mesh.noOfIndices, mesh.baseIndex, mesh.baseVertex);
		++drawCalls;
	}
	return drawCalls;
}

void Mesh::traverseBones(MeshLines& boneData, const aiNode* node, const glm::mat4& parent, const glm::vec3& start, bool traverse) {
	const std::string name(node->mName.data);
	glm::vec3 pos = start;
	glm::mat4 transform = _globalInverseTransform * parent * convert(node->mTransformation);
	const auto iter = _boneMapping.find(name);
	if (iter != _boneMapping.end()) {
		traverse = true;
		pos = glm::column(transform, 3);
		boneData.data.emplace_back(MeshLines::AttributeData{glm::vec4(start.x, start.y, start.z, 1.0f), core::Color::Green});
		boneData.data.emplace_back(MeshLines::AttributeData{glm::vec4(pos.x, pos.y, pos.z, 1.0f), core::Color::Blue});
	}
	if (!traverse) {
		transform = _globalInverseTransform * glm::mat4(1.0f);
	}
	for (uint32_t i = 0u; i < node->mNumChildren; ++i) {
		traverseBones(boneData, node->mChildren[i], transform, pos, traverse);
	}
}

int Mesh::renderBones(video::Shader& shader) {
	core_assert(shader.isActive());

	if (_state != io::IOSTATE_LOADED) {
		return 0;
	}
	setupLineBufferAttributes(shader);

	MeshLines boneData;
	boneData.reserve(_boneMapping.size() * 2);
	traverseBones(boneData, _scene->mRootNode, glm::mat4(1.0f), glm::vec3(0), false);
	_vertexBufferLines.update(_vertexBufferLinesIndex, boneData.data);
	video::ScopedBuffer scopedBuf(_vertexBufferLines);
	ScopedLineWidth lineWidth(2.0f);
	const int elements = _vertexBufferLines.elements(_vertexBufferLinesIndex, 2);
	video::drawArrays(video::Primitive::Lines, elements);

	return 1;
}

int Mesh::renderNormals(video::Shader& shader) {
	core_assert(shader.isActive());

	if (_state != io::IOSTATE_LOADED) {
		return 0;
	}
	setupLineBufferAttributes(shader);

	MeshLines normalData;
	normalData.reserve(_vertices.size() * 2);
	size_t j = 0;
	for (const core::Vertex& v : _vertices) {
		glm::vec4& outVertex = normalData.data[j + 0].vertex;
		glm::vec4& outNormal = normalData.data[j + 1].vertex;
		j += 2;
		for (int i = 0; i < lengthof(v._boneIds); ++i) {
			const float weights = v._boneWeights[i];
			if (weights <= glm::epsilon<float>()) {
				continue;
			}
			const glm::mat4& transform = _boneInfo[v._boneIds[i]].finalTransformation;
			const glm::mat3& rotation = glm::mat3(transform);
			outVertex += transform * glm::vec4(v._pos, 1.0f) * weights;
			outNormal += glm::vec4(rotation * v._norm * weights, 0.0f);
		}
	}
	for (size_t i = 0u; i < normalData.data.size(); i += 2) {
		const MeshLines::AttributeData& pos1 = normalData.data[i + 0];
		MeshLines::AttributeData& pos2 = normalData.data[i + 1];
		pos2.color = glm::vec3(core::Color::Yellow);
		pos2.vertex = pos1.vertex + 0.5f * pos2.vertex;
	}

	_vertexBufferLines.update(_vertexBufferLinesIndex, normalData.data);
	video::ScopedBuffer scopedBuf(_vertexBufferLines);
	ScopedLineWidth lineWidth(2.0f);
	const int elements = _vertexBufferLines.elements(_vertexBufferLinesIndex, 2);
	video::drawArrays(video::Primitive::Lines, elements);

	return 1;
}

int Mesh::animations() const {
	if (_scene == nullptr) {
		return -1;
	}
	return _scene->mNumAnimations;
}

}
