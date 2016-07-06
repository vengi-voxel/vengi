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
