#pragma once

#include "video/ShapeBuilder.h"
#include "video/VertexBuffer.h"
#include "video/Camera.h"
#include "video/Types.h"
#include "ColorShader.h"
#include "ColorInstancedShader.h"

namespace frontend {

/**
 * @brief Renderer for the shapes that you can build with the ShapeBuilder.
 *
 * @see video::ShapeBuilder
 * @see video::VertexBuffer
 */
class ShapeRenderer {
public:
	static constexpr int MAX_MESHES = 16;
private:
	video::VertexBuffer _vbo[MAX_MESHES];
	int32_t _vertexIndex[MAX_MESHES];
	int32_t _indexIndex[MAX_MESHES];
	int32_t _colorIndex[MAX_MESHES];
	// for instancing
	int32_t _offsetIndex[MAX_MESHES];
	int32_t _amounts[MAX_MESHES];
	video::Primitive _primitives[MAX_MESHES];
	uint32_t _currentMeshIndex = 0u;
	shader::ColorShader& _colorShader;
	shader::ColorInstancedShader& _colorInstancedShader;

public:
	ShapeRenderer();
	~ShapeRenderer();

	bool init();

	bool deleteMesh(int32_t meshIndex);

	/**
	 * @param[in,out meshIndex If this is -1 a new mesh is created. The mesh index is 'returned' here. If
	 * this is a valid mesh index, the mesh is updated with the new data from the @c ShapeBuilder
	 */
	void createOrUpdate(int32_t& meshIndex, const video::ShapeBuilder& shapeBuilder);

	int32_t create(const video::ShapeBuilder& shapeBuilder);

	void shutdown();

	void update(uint32_t meshIndex, const video::ShapeBuilder& shapeBuilder);

	template<class T>
	void updatePositions(uint32_t meshIndex, const std::vector<glm::vec<3, T>>& positions) {
		updatePositions(meshIndex, glm::value_ptr(positions.front()), positions.size() * 3 * sizeof(T), 3);
	}

	void updatePositions(uint32_t meshIndex, const void* posBuf, const size_t posBufLength, const int posBufComponents);

	void render(uint32_t meshIndex, const video::Camera& camera, const glm::mat4& model = glm::mat4()) const;

	void renderAll(const video::Camera& camera, const glm::mat4& model = glm::mat4()) const;
};

}
