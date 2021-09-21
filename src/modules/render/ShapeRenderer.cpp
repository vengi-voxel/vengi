/**
 * @file
 */
#include "ShapeRenderer.h"
#include "video/Renderer.h"
#include "core/Vector.h"
#include "video/Shader.h"
#include "video/ScopedPolygonMode.h"
#include "core/Log.h"
#include "video/Camera.h"
#include <glm/gtc/type_ptr.hpp>

namespace render {

ShapeRenderer::ShapeRenderer() :
		_colorShader(shader::ColorShader::getInstance()),
		_colorInstancedShader(shader::ColorInstancedShader::getInstance()),
		_textureShader(shader::TextureShader::getInstance()){
	for (int i = 0; i < MAX_MESHES; ++i) {
		_vertexIndex[i] = -1;
		_indexIndex[i] = -1;
		_offsetIndex[i] = -1;
		_amounts[i] = -1;
		_primitives[i] = video::Primitive::Triangles;
	}
}

ShapeRenderer::~ShapeRenderer() {
	core_assert_msg(_currentMeshIndex == 0, "ShapeRenderer::shutdown() wasn't called");
}

bool ShapeRenderer::init() {
	core_assert_msg(_currentMeshIndex == 0, "ShapeRenderer was already in use");
	if (!_textureShader.setup()) {
		Log::error("Failed to setup texture shader");
		return false;
	}
	if (!_colorShader.setup()) {
		Log::error("Failed to setup color shader");
		return false;
	}
	if (!_colorInstancedShader.setup()) {
		Log::error("Failed to setup color instance shader");
		return false;
	}
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
	_offsetIndex[meshIndex] = -1;
	_amounts[meshIndex] = -1;
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
	const video::ShapeBuilder::Texcoords& uvs = shapeBuilder.getTexcoords();
	_texcoords[meshIndex] = !uvs.empty();
	if (_texcoords[meshIndex] && _texunits[meshIndex] != video::TextureUnit::Max) {
		video::Attribute attributeUV = _textureShader.getTexcoordAttribute(_vertexIndex[meshIndex], &Vertex::uv);
		core_assert_always(_vbo[meshIndex].addAttribute(attributeUV));

		video::Attribute attributePos = _textureShader.getPosAttribute(_vertexIndex[meshIndex], &Vertex::pos);
		core_assert_always(_vbo[meshIndex].addAttribute(attributePos));

		video::Attribute attributeColor = _textureShader.getColorAttribute(_vertexIndex[meshIndex], &Vertex::color);
		core_assert_always(_vbo[meshIndex].addAttribute(attributeColor));
	} else {
		video::Attribute attributePos = _colorShader.getPosAttribute(_vertexIndex[meshIndex], &Vertex::pos);
		// both shaders must have these at the same location
		core_assert(attributePos.location == _colorInstancedShader.getLocationPos());
		core_assert(attributePos.size == _colorInstancedShader.getComponentsPos());
		core_assert_always(_vbo[meshIndex].addAttribute(attributePos));

		video::Attribute attributeColor = _colorShader.getColorAttribute(_vertexIndex[meshIndex], &Vertex::color);
		// both shaders must have these at the same location
		core_assert(attributeColor.location == _colorInstancedShader.getLocationColor());
		core_assert(attributeColor.size == _colorInstancedShader.getComponentsColor());
		core_assert_always(_vbo[meshIndex].addAttribute(attributeColor));
	}

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
	vbo.update(_indexIndex[meshIndex], indicesData, indices.size() * sizeof(video::ShapeBuilder::Indices::value_type));
	const video::ShapeBuilder::Texcoords& uvs = shapeBuilder.getTexcoords();
	_texcoords[meshIndex] = !uvs.empty();
	_primitives[meshIndex] = shapeBuilder.primitive();
}

bool ShapeRenderer::updatePositions(uint32_t meshIndex, const core::DynamicArray<glm::vec3>& positions) {
	const float* posBuf = nullptr;
	if (!positions.empty()) {
		posBuf = glm::value_ptr(positions.front());
	}
	return updatePositions(meshIndex, posBuf, positions.bytes());
}

bool ShapeRenderer::updatePositions(uint32_t meshIndex, const float* posBuf, size_t posBufLength) {
	if (meshIndex >= MAX_MESHES) {
		Log::warn("Invalid mesh index given: %u", meshIndex);
		return false;
	}
	video::Buffer& vbo = _vbo[meshIndex];
	if (_offsetIndex[meshIndex] == -1) {
		_offsetIndex[meshIndex] = vbo.create(posBuf, posBufLength);
		if (_offsetIndex[meshIndex] == -1) {
			return false;
		}
		vbo.setMode(_offsetIndex[meshIndex], video::BufferMode::Stream);

		// TODO: this looks broken... somehow
		video::Attribute attributeOffset = _colorInstancedShader.getOffsetAttribute(_offsetIndex[meshIndex], &glm::vec3::x);
		attributeOffset.divisor = 1;
		attributeOffset.stride = (int)(attributeOffset.size * sizeof(float));
		core_assert_always(_vbo[meshIndex].addAttribute(attributeOffset));
	} else {
		core_assert_always(vbo.update(_offsetIndex[meshIndex], posBuf, posBufLength));
	}
	_amounts[meshIndex] = posBufLength / (_colorInstancedShader.getComponentsOffset() * sizeof(float));
	return true;
}

void ShapeRenderer::shutdown() {
	_textureShader.shutdown();
	_colorShader.shutdown();
	_colorInstancedShader.shutdown();
	for (uint32_t i = 0u; i < _currentMeshIndex; ++i) {
		deleteMesh(i);
	}
	_currentMeshIndex = 0u;
}

void ShapeRenderer::setTextureUnit(uint32_t meshIndex, video::TextureUnit unit) {
	if (meshIndex >= MAX_MESHES) {
		Log::warn("Invalid mesh index given: %u", meshIndex);
		return;
	}
	_texunits[meshIndex] = unit;
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
	cnt += renderAllInstanced(camera, model);
	cnt += renderAllTextured(camera, model);
	cnt += renderAllColored(camera, model);
	return cnt;
}

int ShapeRenderer::renderAllInstanced(const video::Camera& camera, const glm::mat4& model) const {
	int cnt = 0;
	for (uint32_t meshIndex = 0u; meshIndex < _currentMeshIndex; ++meshIndex) {
		if (_vertexIndex[meshIndex] == -1) {
			continue;
		}
		if (_hidden[meshIndex]) {
			continue;
		}
		if (_amounts[meshIndex] <= 0) {
			continue;
		}
		core_assert(_offsetIndex[meshIndex] != -1);
		if (!_colorInstancedShader.isActive()) {
			// TODO: instanced texture shader?
			_colorInstancedShader.activate();
			core_assert_always(_colorInstancedShader.setViewprojection(camera.viewProjectionMatrix()));
			core_assert_always(_colorInstancedShader.setModel(model));
		}
		core_assert_always(_vbo[meshIndex].bind());
		const uint32_t indices = _vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(video::ShapeBuilder::Indices::value_type));
		video::drawElementsInstanced<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices, _amounts[meshIndex]);
		++cnt;
	}
	if (_colorInstancedShader.isActive()) {
		_colorInstancedShader.deactivate();
	}
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
		if (_amounts[meshIndex] > 0) {
			continue;
		}
		if (_texcoords[meshIndex] && video::currentTexture(_texunits[meshIndex]) != video::InvalidId) {
			continue;
		}
		if (!_colorShader.isActive()) {
			_colorShader.activate();
			core_assert_always(_colorShader.setViewprojection(camera.viewProjectionMatrix()));
			core_assert_always(_colorShader.setModel(model));
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

int ShapeRenderer::renderAllTextured(const video::Camera& camera, const glm::mat4& model) const {
	int cnt = 0;
	for (uint32_t meshIndex = 0u; meshIndex < _currentMeshIndex; ++meshIndex) {
		if (_vertexIndex[meshIndex] == -1) {
			continue;
		}
		if (_hidden[meshIndex]) {
			continue;
		}
		if (_amounts[meshIndex] > 0) {
			continue;
		}
		if (!_texcoords[meshIndex] || video::currentTexture(_texunits[meshIndex]) == video::InvalidId) {
			continue;
		}
		if (!_textureShader.isActive()) {
			_textureShader.activate();
			core_assert_always(_textureShader.setViewprojection(camera.viewProjectionMatrix()));
			core_assert_always(_textureShader.setModel(model));
		}
		_textureShader.setTexture(_texunits[meshIndex]);
		core_assert_always(_vbo[meshIndex].bind());
		const uint32_t indices = _vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(video::ShapeBuilder::Indices::value_type));
		video::drawElements<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices);
		_vbo[meshIndex].unbind();
		++cnt;
	}
	if (_textureShader.isActive()) {
		_textureShader.deactivate();
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

	if (_amounts[meshIndex] > 0) {
		core_assert(_offsetIndex[meshIndex] != -1);
		_colorInstancedShader.activate();
		core_assert_always(_colorInstancedShader.setViewprojection(camera.viewProjectionMatrix()));
		core_assert_always(_colorInstancedShader.setModel(model));
		core_assert_always(_vbo[meshIndex].bind());
		video::drawElementsInstanced<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices, _amounts[meshIndex]);
		_colorInstancedShader.deactivate();
	} else {
		const bool useTexture = _texcoords[meshIndex] && video::currentTexture(_texunits[meshIndex]) != video::InvalidId;
		if (useTexture) {
			_textureShader.activate();
			_textureShader.setTexture(_texunits[meshIndex]);
			core_assert_always(_textureShader.setViewprojection(camera.viewProjectionMatrix()));
			core_assert_always(_textureShader.setModel(model));
		} else {
			_colorShader.activate();
			core_assert_always(_colorShader.setViewprojection(camera.viewProjectionMatrix()));
			core_assert_always(_colorShader.setModel(model));
		}
		core_assert_always(_vbo[meshIndex].bind());
		video::drawElements<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices);
		if (useTexture) {
			_textureShader.deactivate();
		} else {
			_colorShader.deactivate();
		}
	}
	_vbo[meshIndex].unbind();
	return true;
}

}
