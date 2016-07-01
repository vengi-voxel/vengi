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

namespace video {

class Mesh : public io::IOResource {
private:
	struct Vertex {
		glm::vec3 _pos;
		glm::vec3 _norm;
		glm::vec2 _uv;

		Vertex(const glm::vec3& p, const glm::vec3& n, const glm::vec2& t) :
				_pos(p), _norm(n), _uv(t) {
		}
	};
	typedef std::vector<Vertex> Vertices;
	typedef std::vector<uint32_t> Indices;

	void loadTextureImages(const aiScene* scene, const std::string& filename);

	GLuint _vertexArrayObject = 0u;
	GLuint _vbo = 0u;
	GLuint _indexBuffer = 0u;
	bool _readyToInit = false;

	std::vector<GLMeshData> _meshData;
	std::vector<image::ImagePtr> _images;
	std::vector<TexturePtr> _textures;
	Vertices _vertices;
	Indices _indices;
public:
	Mesh();
	~Mesh();

	void shutdown();
	bool loadMesh(const std::string& filename);
	bool initMesh(Shader& shader);
	int render();
};

typedef std::shared_ptr<Mesh> MeshPtr;

}
