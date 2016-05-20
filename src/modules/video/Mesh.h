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
	typedef std::vector<uint32_t> Indices;
	typedef std::vector<glm::vec2> TexCoords;
	typedef std::vector<glm::vec3> Vertices;

	void loadTextureImages(const aiScene* scene, const std::string& filename);

	GLuint _vertexArrayObject;
	// 4 buffers - don't change the order - don't add anything in between these four buffers
	GLuint _posBuffer;
	GLuint _uvBuffer;
	GLuint _normalBuffer;
	GLuint _indexBuffer;
	bool _readyToInit = false;

	std::vector<GLMeshData> _meshData;
	std::vector<image::ImagePtr> _images;
	std::vector<TexturePtr> _textures;

	Vertices _positions;
	Vertices _normals;
	TexCoords _texCoords;
	Indices _indices;

	ShaderPtr _shader;
public:
	Mesh();
	~Mesh();

	void shutdown();
	bool loadMesh(const std::string& filename);
	bool initMesh(const ShaderPtr& shader);
	int render();
};

typedef std::shared_ptr<Mesh> MeshPtr;

}
