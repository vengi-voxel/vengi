#pragma once

#include "video/ShapeBuilder.h"
#include "video/VertexBuffer.h"
#include "video/Camera.h"
#include "ColorShader.h"

namespace frontend {

class ShapeRenderer {
private:
	static constexpr int MAX_MESHES = 16;
	video::VertexBuffer _vbo[MAX_MESHES];
	int32_t _vertexIndex[MAX_MESHES];
	int32_t _indexIndex[MAX_MESHES];
	uint32_t _currentMeshIndex = 0u;
	shader::ColorShader _colorShader;

public:
	bool init();

	bool deleteMesh(uint32_t meshIndex);

	int32_t createMesh(const video::ShapeBuilder& shapeBuilder);

	void shutdown();

	void update(uint32_t meshIndex, const video::ShapeBuilder& shapeBuilder);

	void render(uint32_t meshIndex, const video::Camera& camera, GLenum drawmode = GL_TRIANGLES);

	void renderAll(const video::Camera& camera, GLenum drawmode = GL_TRIANGLES);
};

}
