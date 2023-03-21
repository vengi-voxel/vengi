/**
 * @file
 */
#include "ShapeRenderer.h"
#include "ColorData.h"
#include "video/Renderer.h"
#include "video/Shader.h"
#include "video/ScopedPolygonMode.h"
#include "core/Log.h"
#include "video/Camera.h"
#include <glm/gtc/type_ptr.hpp>

namespace render {

ShapeRenderer::ShapeRenderer() :
		_colorShader(shader::ColorShader::getInstance()) {
	for (int i = 0; i < MAX_MESHES; ++i) {
		_vertexIndex[i] = -1;
		_indexIndex[i] = -1;
		_primitives[i] = video::Primitive::Triangles;
	}
}

ShapeRenderer::~ShapeRenderer() {
	core_assert_msg(_currentMeshIndex == 0, "ShapeRenderer::shutdown() wasn't called");
}

bool ShapeRenderer::init() {
	core_assert_msg(_currentMeshIndex == 0, "ShapeRenderer was already in use");
	if (!_colorShader.setup()) {
		Log::error("Failed to setup color shader");
		return false;
	}

	_uniformBlock.create(_uniformBlockData);

	core_assert_always(_colorShader.setUniformblock(_uniformBlock.getUniformblockUniformBuffer()));
	return true;
}

bool ShapeRenderer::deleteMesh(int32_t meshIndex) {
	if (meshIndex < 0) {
		return false;
	}
	if (_currentMeshIndex < (uint32_t)meshIndex) {
		return false;
	}
	_vbo[meshIndex].shutdown();
	_vertexIndex[meshIndex] = -1;
	_indexIndex[meshIndex] = -1;
	_primitives[meshIndex] = video::Primitive::Triangles;
	if (meshIndex > 0 && (uint32_t)meshIndex == _currentMeshIndex) {
		--_currentMeshIndex;
	}
	return true;
}

void ShapeRenderer::createOrUpdate(int32_t& meshIndex, const video::ShapeBuilder& shapeBuilder) {
	if (meshIndex < 0) {
		meshIndex = create(shapeBuilder);
	} else {
		update(static_cast<uint32_t>(meshIndex), shapeBuilder);
	}
}

int32_t ShapeRenderer::create(const video::ShapeBuilder& shapeBuilder) {
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

	_vertices.clear();
	_vertices.reserve(shapeBuilder.getVertices().size());
	shapeBuilder.iterate([&] (const glm::vec3& pos, const glm::vec2& uv, const glm::vec4& color, const glm::vec3& normal) {
		_vertices.emplace_back(Vertex{glm::vec4(pos, 1.0f), color, uv, normal});
	});
	const void* verticesData = nullptr;
	if (!_vertices.empty()) {
		verticesData = &_vertices.front();
	}
	_vertexIndex[meshIndex] = _vbo[meshIndex].create(verticesData, _vertices.size() * sizeof(Vertex));
	if (_vertexIndex[meshIndex] == -1) {
		Log::error("Could not create vbo for vertices");
		return -1;
	}

	const video::ShapeBuilder::Indices& indices = shapeBuilder.getIndices();
	const void* indicesData = nullptr;
	if (!indices.empty()) {
		indicesData = &indices.front();
	}
	_indexIndex[meshIndex] = _vbo[meshIndex].create(indicesData, indices.size() * sizeof(video::ShapeBuilder::Indices::value_type),
			video::BufferType::IndexBuffer);
	if (_indexIndex[meshIndex] == -1) {
		_vertexIndex[meshIndex] = -1;
		_vbo[meshIndex].shutdown();
		Log::error("Could not create vbo for indices");
		return -1;
	}

	// configure shader attributes
	video::Attribute attributePos = _colorShader.getPosAttribute(_vertexIndex[meshIndex], &Vertex::pos);
	core_assert_always(_vbo[meshIndex].addAttribute(attributePos));

	video::Attribute attributeColor = _colorShader.getColorAttribute(_vertexIndex[meshIndex], &Vertex::color);
	core_assert_always(_vbo[meshIndex].addAttribute(attributeColor));

	_primitives[meshIndex] = shapeBuilder.primitive();

	++_currentMeshIndex;
	return meshIndex;
}

void ShapeRenderer::update(uint32_t meshIndex, const video::ShapeBuilder& shapeBuilder) {
	if (meshIndex >= MAX_MESHES) {
		Log::warn("Invalid mesh index given: %u", meshIndex);
		return;
	}
	_vertices.clear();
	_vertices.reserve(shapeBuilder.getVertices().size());
	shapeBuilder.iterate([&] (const glm::vec3& pos, const glm::vec2& uv, const glm::vec4& color, const glm::vec3& normal) {
		_vertices.emplace_back(Vertex{glm::vec4(pos, 1.0f), color, uv, normal});
	});
	const void* verticesData = nullptr;
	if (!_vertices.empty()) {
		verticesData = &_vertices.front();
	}
	video::Buffer& vbo = _vbo[meshIndex];
	core_assert_always(vbo.update(_vertexIndex[meshIndex], verticesData, _vertices.size() * sizeof(Vertex)));
	const video::ShapeBuilder::Indices& indices = shapeBuilder.getIndices();
	const void* indicesData = nullptr;
	if (!indices.empty()) {
		indicesData = &indices.front();
	}
	core_assert_always(vbo.update(_indexIndex[meshIndex], indicesData, indices.size() * sizeof(video::ShapeBuilder::Indices::value_type)));
	_primitives[meshIndex] = shapeBuilder.primitive();
}

void ShapeRenderer::shutdown() {
	_uniformBlock.shutdown();
	_colorShader.shutdown();
	for (uint32_t i = 0u; i < _currentMeshIndex; ++i) {
		deleteMesh(i);
	}
	_currentMeshIndex = 0u;
}

void ShapeRenderer::hide(int32_t meshIndex, bool hide) {
	if (meshIndex < 0 || meshIndex >= MAX_MESHES) {
		return;
	}
	_hidden[meshIndex] = hide;
}

bool ShapeRenderer::hiddenState(int32_t meshIndex) const {
	if (meshIndex < 0 || meshIndex >= MAX_MESHES) {
		return true;
	}
	return _hidden[meshIndex];
}

int ShapeRenderer::renderAll(const video::Camera& camera, const glm::mat4& model) const {
	int cnt = 0;
	cnt += renderAllColored(camera, model);
	return cnt;
}

int ShapeRenderer::renderAllColored(const video::Camera& camera, const glm::mat4& model) const {
	int cnt = 0;
	for (uint32_t meshIndex = 0u; meshIndex < _currentMeshIndex; ++meshIndex) {
		if (_vertexIndex[meshIndex] == -1) {
			continue;
		}
		if (_hidden[meshIndex]) {
			continue;
		}
		if (!_colorShader.isActive()) {
			_colorShader.activate();
			_uniformBlockData.model = model;
			_uniformBlockData.viewprojection = camera.viewProjectionMatrix();
			core_assert_always(_uniformBlock.update(_uniformBlockData));
			core_assert_always(_colorShader.setUniformblock(_uniformBlock.getUniformblockUniformBuffer()));
		}
		core_assert_always(_vbo[meshIndex].bind());
		const uint32_t indices = _vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(video::ShapeBuilder::Indices::value_type));
		video::drawElements<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices);
		_vbo[meshIndex].unbind();
		++cnt;
	}
	if (_colorShader.isActive()) {
		_colorShader.deactivate();
	}
	return cnt;
}

bool ShapeRenderer::render(uint32_t meshIndex, const video::Camera& camera, const glm::mat4& model) const {
	if (meshIndex == (uint32_t)-1) {
		return false;
	}
	if (meshIndex >= MAX_MESHES) {
		Log::warn("Invalid mesh index given: %u", meshIndex);
		return false;
	}
	if (_vertexIndex[meshIndex] == -1) {
		return false;
	}
	if (_hidden[meshIndex]) {
		return false;
	}

	const uint32_t indices = _vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(video::ShapeBuilder::Indices::value_type));

	_uniformBlockData.model = model;
	_uniformBlockData.viewprojection = camera.viewProjectionMatrix();
	core_assert_always(_uniformBlock.update(_uniformBlockData));
	core_assert_always(_colorShader.activate());
	core_assert_always(_colorShader.setUniformblock(_uniformBlock.getUniformblockUniformBuffer()));
	core_assert_always(_vbo[meshIndex].bind());
	video::drawElements<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices);
	_colorShader.deactivate();
	core_assert_always(_vbo[meshIndex].unbind());
	return true;
}

}
