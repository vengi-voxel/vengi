#pragma once

#include "video/ShapeBuilder.h"
#include "video/VertexBuffer.h"
#include "video/Camera.h"
#include "video/Types.h"
#include "ColorShader.h"

namespace frontend {

/**
 * @brief Renderer for the shapes that you can build with the ShapeBuilder.
 *
 * @see video::ShapeBuilder
 * @see video::VertexBuffer
 */
class ShapeRenderer {
private:
	static constexpr int MAX_MESHES = 16;
	video::VertexBuffer _vbo[MAX_MESHES];
	int32_t _vertexIndex[MAX_MESHES];
	int32_t _indexIndex[MAX_MESHES];
	int32_t _colorIndex[MAX_MESHES];
	video::Primitive _primitives[MAX_MESHES];
	uint32_t _currentMeshIndex = 0u;
	shader::ColorShader& _colorShader;

public:
	ShapeRenderer();
	~ShapeRenderer();

	bool init();

	bool deleteMesh(int32_t meshIndex);

	int32_t createMesh(const video::ShapeBuilder& shapeBuilder);

	void shutdown();

	void update(uint32_t meshIndex, const video::ShapeBuilder& shapeBuilder);

	void render(uint32_t meshIndex, const video::Camera& camera, const glm::mat4& model = glm::mat4()) const;

	void renderAll(const video::Camera& camera, const glm::mat4& model = glm::mat4()) const;
};

}
