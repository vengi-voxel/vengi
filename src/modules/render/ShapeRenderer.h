/**
 * @file
 */

#pragma once

#include "video/ShapeBuilder.h"
#include "video/Buffer.h"
#include "video/Types.h"
#include "video/Shader.h"
#include "core/IComponent.h"
#include "core/collection/DynamicArray.h"
#include "ColorShader.h"

namespace render {

/**
 * @brief Renderer for the shapes that you can build with the ShapeBuilder.
 *
 * @see video::ShapeBuilder
 * @see video::Buffer
 */
class ShapeRenderer : public core::IComponent {
public:
	static constexpr int MAX_MESHES = 16;
private:
	struct Vertex {
		glm::vec4 pos;
		glm::vec4 color;
		glm::vec2 uv;
		glm::vec3 normal;
	};

	video::Buffer _vbo[MAX_MESHES];
	int32_t _vertexIndex[MAX_MESHES];
	bool _hidden[MAX_MESHES] { false };
	int32_t _indexIndex[MAX_MESHES];
	video::Primitive _primitives[MAX_MESHES];
	uint32_t _currentMeshIndex = 0u;
	mutable shader::ColorData _uniformBlock;
	shader::ColorShader& _colorShader;

	core::DynamicArray<Vertex> _vertices;

	int renderAllColored(const video::Camera& camera, const glm::mat4& model) const;

public:
	ShapeRenderer();
	~ShapeRenderer();

	bool init() override;

	bool deleteMesh(int32_t meshIndex);

	/**
	 * @param[in,out] meshIndex If this is -1 a new mesh is created. The mesh index is 'returned' here. If
	 * this is a valid mesh index, the mesh is updated with the new data from the @c ShapeBuilder
	 */
	void createOrUpdate(int32_t& meshIndex, const video::ShapeBuilder& shapeBuilder);

	int32_t create(const video::ShapeBuilder& shapeBuilder);

	void hide(int32_t meshIndex, bool hide);
	bool hiddenState(int32_t meshIndex) const;

	void shutdown() override;

	void update(uint32_t meshIndex, const video::ShapeBuilder& shapeBuilder);

	bool render(uint32_t meshIndex, const video::Camera& camera, const glm::mat4& model = glm::mat4(1.0f)) const;

	int renderAll(const video::Camera& camera, const glm::mat4& model = glm::mat4(1.0f)) const;
};

}
