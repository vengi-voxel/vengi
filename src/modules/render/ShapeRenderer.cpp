/**
 * @file
 */
#include "ShapeRenderer.h"
#include "core/Common.h"
#include "core/Log.h"
#include "video/Camera.h"
#include "video/Renderer.h"
#include "video/ScopedBlendMode.h"
#include "video/ScopedState.h"
#include "video/Types.h"

namespace render {

ShapeRenderer::ShapeRenderer()
	: _colorShader(shader::ColorShader::getInstance()), _lineShader(shader::LineShader::getInstance()) {
	for (int i = 0; i < MAX_MESHES; ++i) {
		_vertexIndex[i] = -1;
		_indexIndex[i] = -1;
		_primitives[i] = video::Primitive::Triangles;
	}
}

ShapeRenderer::~ShapeRenderer() {
	core_assert_msg(_currentMeshIndex == 0, "ShapeRenderer::shutdown() wasn't called");
}

bool ShapeRenderer::isLinePrimitive(video::Primitive primitive) {
	return primitive == video::Primitive::Lines || primitive == video::Primitive::LineStrip;
}

bool ShapeRenderer::init() {
	core_assert_msg(_currentMeshIndex == 0, "ShapeRenderer was already in use");
	if (!_colorShader.setup()) {
		Log::error("Failed to setup color shader");
		return false;
	}
	if (!_lineShader.setup()) {
		Log::error("Failed to setup line shader");
		_colorShader.shutdown();
		return false;
	}
	core_assert_always(_uniformBlock.create(_uniformBlockData));
	core_assert_always(_colorShader.setUniformblock(_uniformBlock.getUniformblockUniformBuffer()));
	core_assert_always(_lineData.create(_lineVertData));
	core_assert_always(_lineShader.setVert(_lineData.getVertUniformBuffer()));

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
	_lineMesh[meshIndex] = false;
	_primitives[meshIndex] = video::Primitive::Triangles;
	if (meshIndex > 0 && (uint32_t)meshIndex == _currentMeshIndex) {
		--_currentMeshIndex;
	}
	return true;
}

void ShapeRenderer::createOrUpdate(int32_t &meshIndex, const video::ShapeBuilder &shapeBuilder) {
	if (meshIndex < 0) {
		meshIndex = create(shapeBuilder);
	} else {
		update(static_cast<uint32_t>(meshIndex), shapeBuilder);
	}
}

void ShapeRenderer::fillVertices(const video::ShapeBuilder &shapeBuilder) {
	_vertices.clear();
	_vertices.reserve(shapeBuilder.getVertices().size());
	shapeBuilder.iterate(
		[&](const glm::vec3 &pos, const glm::vec2 &uv, const glm::vec4 &color, const glm::vec3 &normal) {
			_vertices.emplace_back(Vertex{glm::vec4(pos, 1.0f), color, uv, normal});
		});
}

void ShapeRenderer::emitLineQuad(const Vertex &a, const Vertex &b) {
	const glm::vec3 d = glm::vec3(b.pos) - glm::vec3(a.pos);
	if (glm::dot(d, d) < 1.0e-12f) {
		return;
	}
	const uint32_t base = (uint32_t)_lineVertices.size();
	LineVertex v;
	v.end = glm::vec4(glm::vec3(b.pos), 0.0f);
	v.start = glm::vec4(glm::vec3(a.pos), 0.0f);
	v.color = a.color;
	_lineVertices.push_back(v);
	v.start.w = 1.0f;
	_lineVertices.push_back(v);
	v.start.w = 2.0f;
	v.color = b.color;
	_lineVertices.push_back(v);
	v.start.w = 3.0f;
	_lineVertices.push_back(v);
	_lineIndices.push_back(base + 0);
	_lineIndices.push_back(base + 1);
	_lineIndices.push_back(base + 2);
	_lineIndices.push_back(base + 2);
	_lineIndices.push_back(base + 3);
	_lineIndices.push_back(base + 0);
}

void ShapeRenderer::expandLines(video::Primitive primitive, const video::ShapeBuilder::Indices &indices) {
	_lineVertices.clear();
	_lineIndices.clear();
	if (_vertices.empty() || indices.empty()) {
		return;
	}
	const size_t nverts = _vertices.size();
	_lineVertices.reserve(indices.size() * 2u);
	_lineIndices.reserve(indices.size() * 3u);
	if (primitive == video::Primitive::LineStrip) {
		for (size_t i = 0; i + 1 < indices.size(); ++i) {
			const uint32_t i0 = indices[i];
			const uint32_t i1 = indices[i + 1];
			if (i0 >= nverts || i1 >= nverts) {
				continue;
			}
			emitLineQuad(_vertices[i0], _vertices[i1]);
		}
		return;
	}
	for (size_t i = 0; i + 1 < indices.size(); i += 2) {
		const uint32_t i0 = indices[i];
		const uint32_t i1 = indices[i + 1];
		if (i0 >= nverts || i1 >= nverts) {
			continue;
		}
		emitLineQuad(_vertices[i0], _vertices[i1]);
	}
}

const void *ShapeRenderer::vertexData(bool lines, size_t &bytes) const {
	if (lines) {
		bytes = _lineVertices.size() * sizeof(LineVertex);
		return bytes == 0 ? nullptr : _lineVertices.data();
	}
	bytes = _vertices.size() * sizeof(Vertex);
	return bytes == 0 ? nullptr : _vertices.data();
}

const void *ShapeRenderer::indexData(bool lines, const video::ShapeBuilder &shapeBuilder, size_t &bytes) const {
	if (lines) {
		bytes = _lineIndices.size() * sizeof(uint32_t);
		return bytes == 0 ? nullptr : _lineIndices.data();
	}
	const video::ShapeBuilder::Indices &indices = shapeBuilder.getIndices();
	bytes = indices.size() * sizeof(video::ShapeBuilder::Indices::value_type);
	return bytes == 0 ? nullptr : &indices.front();
}

void ShapeRenderer::configureAttributes(uint32_t meshIndex, bool lines) {
	video::Buffer &vbo = _vbo[meshIndex];
	vbo.clearAttributes();
	vbo.destroyVertexArray();
	if (lines) {
		video::Attribute attributeStart = _lineShader.getStartAttribute(_vertexIndex[meshIndex], &LineVertex::start);
		core_assert_always(vbo.addAttribute(attributeStart));
		video::Attribute attributeEnd = _lineShader.getEndAttribute(_vertexIndex[meshIndex], &LineVertex::end);
		core_assert_always(vbo.addAttribute(attributeEnd));
		video::Attribute attributeColor = _lineShader.getColorAttribute(_vertexIndex[meshIndex], &LineVertex::color);
		core_assert_always(vbo.addAttribute(attributeColor));
		return;
	}
	video::Attribute attributePos = _colorShader.getPosAttribute(_vertexIndex[meshIndex], &Vertex::pos);
	core_assert_always(vbo.addAttribute(attributePos));
	video::Attribute attributeColor = _colorShader.getColorAttribute(_vertexIndex[meshIndex], &Vertex::color);
	core_assert_always(vbo.addAttribute(attributeColor));
	video::Attribute attributeNormal = _colorShader.getNormalAttribute(_vertexIndex[meshIndex], &Vertex::normal);
	if (!vbo.addAttribute(attributeNormal)) {
		Log::debug("Failed to add normal attribute - not used in the shader?");
	}
}

int32_t ShapeRenderer::create(const video::ShapeBuilder &shapeBuilder) {
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

	fillVertices(shapeBuilder);
	const bool lines = isLinePrimitive(shapeBuilder.primitive());
	if (lines) {
		expandLines(shapeBuilder.primitive(), shapeBuilder.getIndices());
	}

	size_t vertexBytes = 0;
	const void *verticesData = vertexData(lines, vertexBytes);
	_vertexIndex[meshIndex] = _vbo[meshIndex].create(verticesData, vertexBytes);
	if (_vertexIndex[meshIndex] == -1) {
		Log::error("Could not create vbo for vertices");
		return -1;
	}

	size_t indexBytes = 0;
	const void *indicesData = indexData(lines, shapeBuilder, indexBytes);
	_indexIndex[meshIndex] = _vbo[meshIndex].create(indicesData, indexBytes, video::BufferType::IndexBuffer);
	if (_indexIndex[meshIndex] == -1) {
		_vertexIndex[meshIndex] = -1;
		_vbo[meshIndex].shutdown();
		Log::error("Could not create vbo for indices");
		return -1;
	}

	configureAttributes(meshIndex, lines);
	_lineMesh[meshIndex] = lines;
	_primitives[meshIndex] = lines ? video::Primitive::Triangles : shapeBuilder.primitive();

	++_currentMeshIndex;
	return meshIndex;
}

void ShapeRenderer::update(uint32_t meshIndex, const video::ShapeBuilder &shapeBuilder) {
	if (meshIndex >= MAX_MESHES) {
		Log::warn("Invalid mesh index given: %u", meshIndex);
		return;
	}
	fillVertices(shapeBuilder);
	const bool lines = isLinePrimitive(shapeBuilder.primitive());
	if (lines) {
		expandLines(shapeBuilder.primitive(), shapeBuilder.getIndices());
	}

	size_t vertexBytes = 0;
	const void *verticesData = vertexData(lines, vertexBytes);
	video::Buffer &vbo = _vbo[meshIndex];
	core_assert_always(vbo.update(_vertexIndex[meshIndex], verticesData, vertexBytes));
	size_t indexBytes = 0;
	const void *indicesData = indexData(lines, shapeBuilder, indexBytes);
	core_assert_always(vbo.update(_indexIndex[meshIndex], indicesData, indexBytes));
	if (_lineMesh[meshIndex] != lines) {
		configureAttributes(meshIndex, lines);
	}
	_lineMesh[meshIndex] = lines;
	_primitives[meshIndex] = lines ? video::Primitive::Triangles : shapeBuilder.primitive();
}

void ShapeRenderer::shutdown() {
	_uniformBlock.shutdown();
	_lineData.shutdown();
	_lineShader.shutdown();
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

void ShapeRenderer::activateColorShader(const video::Camera &camera, const glm::mat4 &model) const {
	if (_lineShader.isActive()) {
		_lineShader.deactivate();
	}
	const bool wasActive = _colorShader.isActive();
	if (!wasActive) {
		_colorShader.activate();
	}
	_uniformBlockData.model = model;
	_uniformBlockData.viewprojection = camera.viewProjectionMatrix();
	// TODO: RENDERER: allow to configure lighting
	// the fourth component is the light intensity - set it to something greater than 0 to active shading
	_uniformBlockData.lightColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	_uniformBlockData.lightPos = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
	core_assert_always(_uniformBlock.update(_uniformBlockData));
	if (!wasActive) {
		core_assert_always(_colorShader.setUniformblock(_uniformBlock.getUniformblockUniformBuffer()));
	}
}

void ShapeRenderer::activateLineShader(const video::Camera &camera, const glm::mat4 &model) const {
	if (_colorShader.isActive()) {
		_colorShader.deactivate();
	}
	const bool wasActive = _lineShader.isActive();
	if (!wasActive) {
		_lineShader.activate();
	}
	const glm::ivec2 &vp = camera.size();
	_lineVertData.viewprojection = camera.viewProjectionMatrix();
	_lineVertData.model = model;
	_lineVertData.viewport =
		glm::vec4((float)core_max(vp.x, 1), (float)core_max(vp.y, 1), _lineWidth, 0.0f);
	core_assert_always(_lineData.update(_lineVertData));
	if (!wasActive) {
		core_assert_always(_lineShader.setVert(_lineData.getVertUniformBuffer()));
	}
}

int ShapeRenderer::renderAll(const video::Camera &camera, const glm::mat4 &model) const {
	int cnt = 0;
	for (uint32_t meshIndex = 0u; meshIndex < _currentMeshIndex; ++meshIndex) {
		if (_vertexIndex[meshIndex] == -1) {
			continue;
		}
		if (_hidden[meshIndex]) {
			continue;
		}
		if (_lineMesh[meshIndex]) {
			activateLineShader(camera, model);
			video::ScopedBlendMode blend(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha,
										 video::BlendEquation::Add);
			video::ScopedState depthTest(video::State::DepthTest, false);
			video::ScopedState depthMask(video::State::DepthMask, false);
			video::ScopedState cull(video::State::CullFace, false);
			core_assert_always(_vbo[meshIndex].bind());
			const uint32_t indices =
				_vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(video::ShapeBuilder::Indices::value_type));
			video::drawElements<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices);
			_vbo[meshIndex].unbind();
		} else {
			activateColorShader(camera, model);
			core_assert_always(_vbo[meshIndex].bind());
			const uint32_t indices =
				_vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(video::ShapeBuilder::Indices::value_type));
			video::drawElements<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices);
			_vbo[meshIndex].unbind();
		}
		++cnt;
	}
	if (_colorShader.isActive()) {
		_colorShader.deactivate();
	}
	if (_lineShader.isActive()) {
		_lineShader.deactivate();
	}
	return cnt;
}

bool ShapeRenderer::render(uint32_t meshIndex, const video::Camera &camera, const glm::mat4 &model) const {
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

	const uint32_t indices =
		_vbo[meshIndex].elements(_indexIndex[meshIndex], 1, sizeof(video::ShapeBuilder::Indices::value_type));
	if (indices == 0) {
		return false;
	}
	if (_lineMesh[meshIndex]) {
		activateLineShader(camera, model);
		video::ScopedBlendMode blend(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha,
									 video::BlendEquation::Add);
		video::ScopedState depthTest(video::State::DepthTest, false);
		video::ScopedState depthMask(video::State::DepthMask, false);
		video::ScopedState cull(video::State::CullFace, false);
		core_assert_always(_vbo[meshIndex].bind());
		video::drawElements<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices);
		_lineShader.deactivate();
		core_assert_always(_vbo[meshIndex].unbind());
		return true;
	}
	activateColorShader(camera, model);
	core_assert_always(_vbo[meshIndex].bind());
	video::drawElements<video::ShapeBuilder::Indices::value_type>(_primitives[meshIndex], indices);
	_colorShader.deactivate();
	core_assert_always(_vbo[meshIndex].unbind());
	return true;
}

} // namespace render
