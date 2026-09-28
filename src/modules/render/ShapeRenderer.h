/**
 * @file
 */

#pragma once

#include "video/ShapeBuilder.h"
#include "video/Buffer.h"
#include "video/Types.h"
#include "video/Shader.h"
#include "core/Common.h"
#include "core/IComponent.h"
#include "core/collection/Buffer.h"
#include "ColorShader.h"
#include "LineShader.h"
#include "LineData.h"

namespace render {

/**
 * @brief Renderer for the shapes that you can build with the ShapeBuilder.
 *
 * Line primitives are expanded to screen-space quads so they stay about one
 * pixel wide with fragment anti-aliasing. Triangle meshes still use ColorShader.
 *
 * @see video::ShapeBuilder
 * @see video::Buffer
 */
class ShapeRenderer : public core::IComponent {
public:
	static constexpr int MAX_MESHES = 2048;
private:
	struct Vertex {
		glm::vec4 pos;
		glm::vec4 color;
		glm::vec2 uv;
		glm::vec3 normal;
	};
	struct LineVertex {
		glm::vec4 start;
		glm::vec4 end;
		glm::vec4 color;
	};

	video::Buffer _vbo[MAX_MESHES];
	int32_t _vertexIndex[MAX_MESHES];
	bool _hidden[MAX_MESHES] { false };
	bool _lineMesh[MAX_MESHES] { false };
	int32_t _indexIndex[MAX_MESHES];
	video::Primitive _primitives[MAX_MESHES];
	uint32_t _currentMeshIndex = 0u;
	float _lineWidth = 2.0f;
	alignas(16) mutable shader::ColorData::UniformblockData _uniformBlockData;
	mutable shader::ColorData _uniformBlock;
	alignas(16) mutable shader::LineData::VertData _lineVertData{};
	mutable shader::LineData _lineData;
	shader::ColorShader& _colorShader;
	shader::LineShader& _lineShader;

	core::Buffer<Vertex> _vertices;
	core::Buffer<LineVertex> _lineVertices;
	core::Buffer<uint32_t> _lineIndices;

	static bool isLinePrimitive(video::Primitive primitive);
	void fillVertices(const video::ShapeBuilder &shapeBuilder);
	void expandLines(video::Primitive primitive, const video::ShapeBuilder::Indices &indices);
	void emitLineQuad(const Vertex &a, const Vertex &b);
	void configureAttributes(uint32_t meshIndex, bool lines);
	const void *vertexData(bool lines, size_t &bytes) const;
	const void *indexData(bool lines, const video::ShapeBuilder &shapeBuilder, size_t &bytes) const;
	void activateColorShader(const video::Camera &camera, const glm::mat4 &model) const;
	void activateLineShader(const video::Camera &camera, const glm::mat4 &model) const;

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

	void setLineWidth(float width);
	float lineWidth() const;

	void shutdown() override;

	void update(uint32_t meshIndex, const video::ShapeBuilder& shapeBuilder);

	bool render(uint32_t meshIndex, const video::Camera& camera, const glm::mat4& model = glm::mat4(1.0f)) const;

	int renderAll(const video::Camera& camera, const glm::mat4& model = glm::mat4(1.0f)) const;
};

inline void ShapeRenderer::setLineWidth(float width) {
	_lineWidth = core_max(width, 1.0f);
}

inline float ShapeRenderer::lineWidth() const {
	return _lineWidth;
}

}
