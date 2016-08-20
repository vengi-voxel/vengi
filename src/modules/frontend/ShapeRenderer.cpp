#include "ShapeRenderer.h"
#include "video/GLFunc.h"
#include "video/Shader.h"

namespace frontend {

bool ShapeRenderer::init() {
	if (!_colorShader.setup()) {
		return false;
	}
	return true;
}

int32_t ShapeRenderer::createMesh(const video::ShapeBuilder& shapeBuilder) {
	if (_currentMeshIndex >= MAX_MESHES) {
		Log::error("Max meshes exceeded");
		return -1;
	}

	std::vector<glm::vec4> vertices;
	shapeBuilder.convertVertices(vertices);
	_vertexIndex[_currentMeshIndex] = _vbo[_currentMeshIndex].create(&vertices[0], core::vectorSize(vertices));
	if (_vertexIndex[_currentMeshIndex] == -1) {
		Log::error("Could not create vbo for vertices");
		return -1;
	}

	const video::ShapeBuilder::Indices& indices= shapeBuilder.getIndices();
	_indexIndex[_currentMeshIndex] = _vbo[_currentMeshIndex].create(&indices[0], core::vectorSize(indices), GL_ELEMENT_ARRAY_BUFFER);
	if (_indexIndex[_currentMeshIndex] == -1) {
		Log::error("Could not create vbo for indices");
		return -1;
	}

	const video::ShapeBuilder::Colors& colors = shapeBuilder.getColors();
	const int32_t cIndex = _vbo[_currentMeshIndex].create(&colors[0], core::vectorSize(colors));
	if (cIndex == -1) {
		Log::error("Could not create vbo for color");
		return -1;
	}

	// configure shader attributes
	core_assert_always(_vbo[_currentMeshIndex].addAttribute(_colorShader.getLocationPos(), _vertexIndex[_currentMeshIndex], 4));
	core_assert_always(_vbo[_currentMeshIndex].addAttribute(_colorShader.getLocationColor(), cIndex, 4));

	const int index = _currentMeshIndex;
	++_currentMeshIndex;
	return index;
}

void ShapeRenderer::shutdown() {
	_colorShader.shutdown();
	for (uint32_t i = 0u; i < _currentMeshIndex; ++i) {
		_vbo[i].shutdown();
	}
	_currentMeshIndex = 0u;
}

void ShapeRenderer::update(uint32_t meshIndex, const video::ShapeBuilder& shapeBuilder) {
	std::vector<glm::vec4> vertices;
	shapeBuilder.convertVertices(vertices);
	core_assert_always(_vbo[meshIndex].update(_vertexIndex[meshIndex], &vertices[0], core::vectorSize(vertices)));
}

void ShapeRenderer::renderAll(const video::Camera& camera, GLenum drawmode) {
	video::ScopedShader scoped(_colorShader);
	core_assert_always(_colorShader.setView(camera.viewMatrix()));
	core_assert_always(_colorShader.setProjection(camera.projectionMatrix()));

	for (uint32_t meshIndex = 0u; meshIndex < _currentMeshIndex; ++meshIndex) {
		core_assert_always(_vbo[meshIndex].bind());
		const GLuint indices = _vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(uint32_t));
		glDrawElements(drawmode, indices, GL_UNSIGNED_INT, 0);
		_vbo[meshIndex].unbind();
	}
	GL_checkError();
}

void ShapeRenderer::render(uint32_t meshIndex, const video::Camera& camera, GLenum drawmode) {
	video::ScopedShader scoped(_colorShader);
	core_assert_always(_colorShader.setView(camera.viewMatrix()));
	core_assert_always(_colorShader.setProjection(camera.projectionMatrix()));

	core_assert_always(_vbo[meshIndex].bind());
	const GLuint indices = _vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(uint32_t));
	glDrawElements(drawmode, indices, GL_UNSIGNED_INT, 0);
	_vbo[meshIndex].unbind();
	GL_checkError();
}

}
