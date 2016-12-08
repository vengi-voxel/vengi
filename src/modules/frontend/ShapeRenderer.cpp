#include "ShapeRenderer.h"
#include "video/GLFunc.h"
#include "video/Shader.h"
#include "video/ScopedPolygonMode.h"

namespace frontend {

ShapeRenderer::ShapeRenderer() :
		_colorShader(shader::ColorShader::getInstance()) {
}

bool ShapeRenderer::init() {
	if (!_colorShader.setup()) {
		return false;
	}
	return true;
}

bool ShapeRenderer::deleteMesh(uint32_t meshIndex) {
	if (_currentMeshIndex <= meshIndex) {
		return false;
	}
	_vbo[meshIndex].shutdown();
	_vertexIndex[meshIndex] = -1;
	_indexIndex[meshIndex] = -1;
	_colorIndex[meshIndex] = -1;
	_primitives[meshIndex] = video::Primitive::Triangles;
	return true;
}

int32_t ShapeRenderer::createMesh(const video::ShapeBuilder& shapeBuilder) {
	uint32_t meshIndex = _currentMeshIndex;
	for (uint32_t i = 0u; i < _currentMeshIndex; ++i) {
		if (!_vbo[i].isValid(0)) {
			meshIndex = i;
			break;
		}
	}

	if (meshIndex >= MAX_MESHES) {
		Log::error("Max meshes exceeded");
		return -1;
	}

	std::vector<glm::vec4> vertices;
	shapeBuilder.convertVertices(vertices);
	_vertexIndex[meshIndex] = _vbo[meshIndex].create(vertices);
	if (_vertexIndex[meshIndex] == -1) {
		Log::error("Could not create vbo for vertices");
		return -1;
	}

	const video::ShapeBuilder::Indices& indices= shapeBuilder.getIndices();
	_indexIndex[meshIndex] = _vbo[meshIndex].create(indices, GL_ELEMENT_ARRAY_BUFFER);
	if (_indexIndex[meshIndex] == -1) {
		_vbo[meshIndex].shutdown();
		Log::error("Could not create vbo for indices");
		return -1;
	}

	const video::ShapeBuilder::Colors& colors = shapeBuilder.getColors();
	if (_colorShader.getComponentsColor() == 4) {
		_colorIndex[meshIndex] = _vbo[meshIndex].create(colors);
	} else {
		core_assert(_colorShader.getComponentsColor() == 3);
		std::vector<glm::vec3> colors3;
		colors3.reserve(colors.size());
		for (const auto c : colors) {
			colors3.push_back(glm::vec3(c));
		}
		_colorIndex[meshIndex] = _vbo[meshIndex].create(colors3);
	}
	if (_colorIndex[meshIndex] == -1) {
		_vbo[meshIndex].shutdown();
		Log::error("Could not create vbo for color");
		return -1;
	}

	// configure shader attributes
	video::VertexBuffer::Attribute attributePos;
	attributePos.bufferIndex = _vertexIndex[meshIndex];
	attributePos.index = _colorShader.getLocationPos();
	attributePos.size = _colorShader.getComponentsPos();
	_vbo[meshIndex].addAttribute(attributePos);

	video::VertexBuffer::Attribute attributeColor;
	attributeColor.bufferIndex = _colorIndex[meshIndex];
	attributeColor.index = _colorShader.getLocationColor();
	attributeColor.size = _colorShader.getComponentsColor();
	_vbo[meshIndex].addAttribute(attributeColor);

	_primitives[meshIndex] = shapeBuilder.primitive();

	++_currentMeshIndex;
	return meshIndex;
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
	core_assert_always(_vbo[meshIndex].update(_vertexIndex[meshIndex], vertices));
	const video::ShapeBuilder::Indices& indices= shapeBuilder.getIndices();
	_vbo[meshIndex].update(_indexIndex[meshIndex], indices);
	const video::ShapeBuilder::Colors& colors = shapeBuilder.getColors();
	if (_colorShader.getComponentsColor() == 4) {
		_vbo[meshIndex].update(_colorIndex[meshIndex], colors);
	} else {
		core_assert(_colorShader.getComponentsColor() == 3);
		std::vector<glm::vec3> colors3;
		colors3.reserve(colors.size());
		for (const auto c : colors) {
			colors3.push_back(glm::vec3(c));
		}
		_vbo[meshIndex].update(_colorIndex[meshIndex], colors3);
	}
	_primitives[meshIndex] = shapeBuilder.primitive();
}

void ShapeRenderer::renderAll(const video::Camera& camera) const {
	video::ScopedShader scoped(_colorShader);
	core_assert_always(_colorShader.setViewprojection(camera.viewProjectionMatrix()));

	for (uint32_t meshIndex = 0u; meshIndex < _currentMeshIndex; ++meshIndex) {
		if (_vertexIndex[meshIndex] == -1) {
			continue;
		}
		core_assert(_vbo[meshIndex].bind());
		const GLuint indices = _vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(uint32_t));
		glDrawElements(std::enum_value(_primitives[meshIndex]), indices, GL_UNSIGNED_INT, 0);
		_vbo[meshIndex].unbind();
	}
	GL_checkError();
}

void ShapeRenderer::render(uint32_t meshIndex, const video::Camera& camera) const {
	video::ScopedShader scoped(_colorShader);
	core_assert_always(_colorShader.setViewprojection(camera.viewProjectionMatrix()));

	core_assert_always(_vbo[meshIndex].bind());
	const GLuint indices = _vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(uint32_t));
	glDrawElements(std::enum_value(_primitives[meshIndex]), indices, GL_UNSIGNED_INT, 0);
	_vbo[meshIndex].unbind();
	GL_checkError();
}

}
