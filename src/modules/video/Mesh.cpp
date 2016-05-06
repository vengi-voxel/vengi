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
		io::IOResource(), _vertexArrayObject(0u), _posBuffer(0u), _uvBuffer(0u), _normalBuffer(0u), _indexBuffer(0u) {
}

Mesh::~Mesh() {
	_textures.clear();
	// destroy all the 4 buffers at once
	glDeleteBuffers(4, &_posBuffer);
	glDeleteVertexArrays(1, &_vertexArrayObject);
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
	if (scene == nullptr) {
		Log::error("Error parsing '%s': '%s'\n", filename.c_str(), importer.GetErrorString());
		_state = io::IOSTATE_FAILED;
		return false;
	}

	_meshData.resize(scene->mNumMeshes);

	uint32_t numIndices = 0u;
	uint32_t numVertices = 0u;

	for (uint32_t i = 0; i < _meshData.size(); ++i) {
		const aiMesh* mesh = scene->mMeshes[i];
		GLMeshData& meshData = _meshData[i];
		meshData.materialIndex[0] = mesh->mMaterialIndex;
		meshData.noOfIndices[0] = mesh->mNumFaces * 3;
		meshData.baseVertex[0] = numVertices;
		meshData.baseIndex[0] = numIndices;

		numVertices += mesh->mNumVertices;
		numIndices += meshData.noOfIndices[0];
	}

	_positions.reserve(numVertices);
	_normals.reserve(numVertices);
	_texCoords.reserve(numVertices);
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

			_positions.push_back(glm::vec3(pos->x, pos->y, pos->z));
			_normals.push_back(glm::vec3(normal->x, normal->y, normal->z));
			_texCoords.push_back(glm::vec2(texCoord->x, texCoord->y));
		}
	}

	loadTextureImages(scene, filename);
	_readyToInit = true;
	return true;
}

bool Mesh::initMesh(const ShaderPtr& shader) {
	if (_shader)
		return true;
	if (!shader)
		return false;

	if (_state != io::IOSTATE_LOADED) {
		if (!_readyToInit) {
			return false;
		}
		for (const image::ImagePtr& i : _images) {
			if (i && i->isLoading()) {
				return false;
			}
		}

		_textures.resize(_images.size());
		int materialIndex = 0;
		for (const image::ImagePtr& i : _images) {
			_textures[materialIndex++] = createTextureFromImage(i);
		}
		_images.clear();

		_state = io::IOSTATE_LOADED;
	}

	_shader = shader;

	glGenVertexArrays(1, &_vertexArrayObject);
	// generate all the 4 needed buffers at once
	glGenBuffers(4, &_posBuffer);

	glBindVertexArray(_vertexArrayObject);

	glBindBuffer(GL_ARRAY_BUFFER, _posBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(_positions[0]) * _positions.size(), &_positions[0], GL_STATIC_DRAW);
	const int locPos = _shader->enableVertexAttribute("a_pos");
	core_assert(locPos >= 0);
	glVertexAttribPointer(locPos, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, _uvBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(_texCoords[0]) * _texCoords.size(), &_texCoords[0], GL_STATIC_DRAW);
	const int locTexCoords = _shader->enableVertexAttribute("a_texcoords");
	core_assert(locTexCoords >= 0);
	glVertexAttribPointer(locTexCoords, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, _normalBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(_normals[0]) * _normals.size(), &_normals[0], GL_STATIC_DRAW);
	const int locNorm = _shader->enableVertexAttribute("a_norm");
	core_assert(locNorm >= 0);
	glVertexAttribPointer(locNorm, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(_indices[0]) * _indices.size(), &_indices[0], GL_STATIC_DRAW);

	glBindVertexArray(0);

	return GL_checkError() == 0;
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
		if (material->GetTextureCount(texType) <= 0)
			continue;

		aiString path;
		if (material->GetTexture(texType, 0, &path, nullptr, nullptr, nullptr, nullptr, nullptr) != AI_SUCCESS)
			continue;

		std::string p(path.data);

		if (p.substr(0, 2) == ".\\") {
			p = p.substr(2, p.size() - 2);
		}

		const std::string fullPath = dir + "/" + p;
		_images[i] = image::loadImage(fullPath);
	}
}

int Mesh::render() {
	if (_state != io::IOSTATE_LOADED)
		return 0;
	glBindVertexArray(_vertexArrayObject);
	int drawCalls = 0;
	for (const GLMeshData& mesh : _meshData) {
		const uint32_t matIdx = mesh.materialIndex[0];
		if (matIdx < _textures.size() && _textures[matIdx]) {
			_textures[matIdx]->bind();
		}
		glDrawElementsBaseVertex(GL_TRIANGLES, mesh.noOfIndices[0], GL_UNSIGNED_INT, GL_OFFSET_CAST(sizeof(uint32_t) * mesh.baseIndex[0]), mesh.baseVertex[0]);
		++drawCalls;
	}
	return drawCalls;
}

}
