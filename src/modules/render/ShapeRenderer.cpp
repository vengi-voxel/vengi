/**
 * @file
 */
#include "ShapeRenderer.h"
#include "video/Renderer.h"
#include "video/Shader.h"
#include "video/ScopedPolygonMode.h"

namespace render {

ShapeRenderer::ShapeRenderer() :
		_colorShader(shader::ColorShader::getInstance()),
		_colorInstancedShader(shader::ColorInstancedShader::getInstance()) {
	for (int i = 0; i < MAX_MESHES; ++i) {
		_vertexIndex[i] = -1;
		_indexIndex[i] = -1;
		_colorIndex[i] = -1;
		_offsetIndex[i] = -1;
		_amounts[i] = -1;
		_primitives[i] = video::Primitive::Triangles;
	}
}

ShapeRenderer::~ShapeRenderer() {
	core_assert_msg(_currentMeshIndex == 0, "ShapeRenderer::shutdown() wasn't called");
	shutdown();
}

bool ShapeRenderer::init() {
	core_assert_msg(_currentMeshIndex == 0, "ShapeRenderer was already in use");
	if (!_colorShader.setup()) {
		return false;
	}
	if (!_colorInstancedShader.setup()) {
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
	_colorIndex[meshIndex] = -1;
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

	std::vector<glm::vec4> vertices;
	shapeBuilder.convertVertices(vertices);
	_vertexIndex[meshIndex] = _vbo[meshIndex].create(vertices);
	if (_vertexIndex[meshIndex] == -1) {
		Log::error("Could not create vbo for vertices");
		return -1;
	}

	const video::ShapeBuilder::Indices& indices = shapeBuilder.getIndices();
	_indexIndex[meshIndex] = _vbo[meshIndex].create(indices, video::BufferType::IndexBuffer);
	if (_indexIndex[meshIndex] == -1) {
		_vertexIndex[meshIndex] = -1;
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
		_vertexIndex[meshIndex] = -1;
		_indexIndex[meshIndex] = -1;
		_vbo[meshIndex].shutdown();
		Log::error("Could not create vbo for color");
		return -1;
	}

	// configure shader attributes
	video::Attribute attributePos = _colorShader.getPosAttribute(_vertexIndex[meshIndex]);
	core_assert(attributePos.index == _colorInstancedShader.getLocationPos());
	core_assert(attributePos.size == _colorInstancedShader.getComponentsPos());
	core_assert_always(_vbo[meshIndex].addAttribute(attributePos));

	video::Attribute attributeColor = _colorShader.getColorAttribute(_colorIndex[meshIndex]);
	core_assert(attributeColor.index == _colorInstancedShader.getLocationColor());
	core_assert(attributeColor.size == _colorInstancedShader.getComponentsColor());
	core_assert_always(_vbo[meshIndex].addAttribute(attributeColor));

	_primitives[meshIndex] = shapeBuilder.primitive();

	++_currentMeshIndex;
	return meshIndex;
}

void ShapeRenderer::shutdown() {
	_colorShader.shutdown();
	_colorInstancedShader.shutdown();
	for (uint32_t i = 0u; i < _currentMeshIndex; ++i) {
		deleteMesh(i);
	}
	_currentMeshIndex = 0u;
}

void ShapeRenderer::update(uint32_t meshIndex, const video::ShapeBuilder& shapeBuilder) {
	std::vector<glm::vec4> vertices;
	shapeBuilder.convertVertices(vertices);
	video::Buffer& vbo = _vbo[meshIndex];
	core_assert_always(vbo.update(_vertexIndex[meshIndex], vertices));
	const video::ShapeBuilder::Indices& indices= shapeBuilder.getIndices();
	vbo.update(_indexIndex[meshIndex], indices);
	const video::ShapeBuilder::Colors& colors = shapeBuilder.getColors();
	if (_colorShader.getComponentsColor() == 4) {
		vbo.update(_colorIndex[meshIndex], colors);
	} else {
		core_assert(_colorShader.getComponentsColor() == 3);
		std::vector<glm::vec3> colors3;
		colors3.reserve(colors.size());
		for (const auto c : colors) {
			colors3.push_back(glm::vec3(c));
		}
		vbo.update(_colorIndex[meshIndex], colors3);
	}
	_primitives[meshIndex] = shapeBuilder.primitive();
}

bool ShapeRenderer::updatePositions(uint32_t meshIndex, const std::vector<glm::vec3>& positions) {
	return updatePositions(meshIndex, glm::value_ptr(positions.front()), core::vectorSize(positions));
}

bool ShapeRenderer::updatePositions(uint32_t meshIndex, const float* posBuf, size_t posBufLength) {
	video::Buffer& vbo = _vbo[meshIndex];
	if (_offsetIndex[meshIndex] == -1) {
		_offsetIndex[meshIndex] = vbo.create(posBuf, posBufLength);
		if (_offsetIndex[meshIndex] == -1) {
			return false;
		}
		vbo.setMode(_offsetIndex[meshIndex], video::BufferMode::Stream);

		// TODO: this looks broken... somehow
		video::Attribute attributeOffset = _colorInstancedShader.getOffsetAttribute(_offsetIndex[meshIndex]);
		attributeOffset.divisor = 1;
		attributeOffset.stride = attributeOffset.size * sizeof(float);
		core_assert_always(_vbo[meshIndex].addAttribute(attributeOffset));
	} else {
		core_assert_always(vbo.update(_offsetIndex[meshIndex], posBuf, posBufLength));
	}
	_amounts[meshIndex] = posBufLength / (_colorInstancedShader.getComponentsOffset() * sizeof(float));
	return true;
}

int ShapeRenderer::renderAll(const video::Camera& camera, const glm::mat4& model, video::Shader* shader) const {
	int cnt = 0;
	for (uint32_t meshIndex = 0u; meshIndex < _currentMeshIndex; ++meshIndex) {
		if (_vertexIndex[meshIndex] == -1) {
			continue;
		}
		if (render(meshIndex, camera, model, shader)) {
			++cnt;
		}
	}
	return cnt;
}

bool ShapeRenderer::render(uint32_t meshIndex, const video::Camera& camera, const glm::mat4& model, video::Shader* shader) const {
	if (meshIndex == (uint32_t)-1) {
		return false;
	}
	core_assert_always(_vbo[meshIndex].bind());
	const uint32_t indices = _vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(video::ShapeBuilder::Indices::value_type));

	if (_amounts[meshIndex] > 0) {
		core_assert(_offsetIndex[meshIndex] != -1);
		if (shader == nullptr) {
			video::ScopedShader scoped(_colorInstancedShader);
			core_assert_always(_colorInstancedShader.setViewprojection(camera.viewProjectionMatrix()));
			core_assert_always(_colorInstancedShader.setModel(model));
		}
		video::drawElementsInstanced<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices, _amounts[meshIndex]);
	} else {
		if (shader == nullptr) {
			video::ScopedShader scoped(_colorShader);
			core_assert_always(_colorShader.setViewprojection(camera.viewProjectionMatrix()));
			core_assert_always(_colorShader.setModel(model));
		}
		video::drawElements<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices);
	}
	_vbo[meshIndex].unbind();
	return true;
}

}
