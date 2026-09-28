/**
 * @file
 */

#include "GridRenderer.h"
#include "color/Color.h"
#include "color/ColorUtil.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "math/AABB.h"
#include "video/Camera.h"
#include "video/Renderer.h"
#include "video/ScopedBlendMode.h"
#include "video/ScopedFaceCull.h"
#include "video/ScopedState.h"
#include "core/GLM.h"
#include "core/GLMConst.h"

namespace render {

GridRenderer::GridRenderer(bool renderAABB, bool renderGrid, bool renderPlane)
	: _gridShader(shader::GridShader::getInstance()), _renderAABB(renderAABB), _renderGrid(renderGrid),
	  _renderPlane(renderPlane) {
}

bool GridRenderer::init() {
	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}
	if (!_gridShader.setup()) {
		Log::error("Failed to initialize the grid shader");
		return false;
	}
	core_assert_always(_gridData.create(_vertData));
	core_assert_always(_gridData.create(_fragData));
	return true;
}

bool GridRenderer::setGridResolution(const glm::ivec3 &resolution) {
	if (resolution.x < 1 || resolution.y < 1 || resolution.z < 1) {
		return false;
	}
	if (_resolution == resolution) {
		return false;
	}
	_resolution = resolution;
	_dirty = true;
	_dirtyPlane = true;
	return true;
}

bool GridRenderer::setGridResolution(int resolution) {
	return setGridResolution(glm::ivec3(resolution));
}

const glm::ivec3 &GridRenderer::gridResolution() const {
	return _resolution;
}

void GridRenderer::setColor(const glm::vec4 &color) {
	if (_shapeBuilder.setColor(color)) {
		_color = color;
		_dirty = true;
	}
}

void GridRenderer::createForwardArrow(const math::AABB<float> &aabb) {
	if (!aabb.isValid() || aabb.isEmpty()) {
		return;
	}
	_shapeBuilder.clear();
	const float arrowSize = 10.0f;
	const float forward = glm::forward().z * arrowSize;
	const float left = glm::left().x * arrowSize;
	const float right = glm::right().x * arrowSize;
	const float arrowX = aabb.getCenterX();
	const float arrowY = aabb.getLowerY();
	const float arrowZ = aabb.getLowerZ();
	const glm::vec3 point1{arrowX + left, arrowY, arrowZ + forward};
	const glm::vec3 point2{arrowX, arrowY, arrowZ + 2.0f * forward};
	const glm::vec3 point3{arrowX + right, arrowY, arrowZ + forward};
	_shapeBuilder.arrow(point1, point2, point3);
	_shapeRenderer.createOrUpdate(_arrow, _shapeBuilder);
	_shapeRenderer.hide(_arrow, true);
}

bool GridRenderer::uploadMesh(video::Buffer &vbo, int32_t &vertexIndex, int32_t &indexIndex,
							  const video::ShapeBuilder &builder) {
	const video::ShapeBuilder::Vertices &vertices = builder.getVertices();
	const video::ShapeBuilder::Indices &indices = builder.getIndices();
	if (vertices.empty() || indices.empty()) {
		return false;
	}
	if (vertexIndex == -1) {
		vertexIndex = vbo.create(vertices.data(), vertices.size() * sizeof(glm::vec3));
		if (vertexIndex == -1) {
			Log::error("Failed to create grid vertex buffer");
			return false;
		}
		vbo.setMode(vertexIndex, video::BufferMode::Dynamic);
		indexIndex = vbo.create(indices.data(), indices.size() * sizeof(video::ShapeBuilder::Indices::value_type),
								video::BufferType::IndexBuffer);
		if (indexIndex == -1) {
			Log::error("Failed to create grid index buffer");
			vertexIndex = -1;
			vbo.shutdown();
			return false;
		}
		vbo.setMode(indexIndex, video::BufferMode::Dynamic);
		core_assert_always(vbo.addAttribute(_gridShader.getPosAttribute(vertexIndex, &glm::vec3::x)));
	} else {
		core_assert_always(vbo.update(vertexIndex, vertices.data(), vertices.size() * sizeof(glm::vec3)));
		core_assert_always(vbo.update(indexIndex, indices.data(),
									  indices.size() * sizeof(video::ShapeBuilder::Indices::value_type)));
	}
	return true;
}

void GridRenderer::drawMesh(video::Buffer &vbo, int32_t vertexIndex, int32_t indexIndex, const video::Camera &camera,
							const glm::mat4 &model, const glm::vec3 &mins, const glm::vec3 &step,
							float majorStride) {
	if (vertexIndex < 0 || indexIndex < 0) {
		return;
	}
	const uint32_t indices =
		vbo.elements(indexIndex, 1, sizeof(video::ShapeBuilder::Indices::value_type));
	if (indices == 0) {
		return;
	}

	_vertData.viewprojection = camera.viewProjectionMatrix();
	_vertData.model = model;
	_fragData.mins = glm::vec4(mins, 0.0f);
	_fragData.step = glm::vec4(step, majorStride);
	const glm::vec4 minor = color::darker(_color, 1.4f);
	_fragData.color = minor;
	_fragData.majorcolor = (majorStride <= 1.0001f) ? minor : glm::mix(minor, _color, 0.28f);
	core_assert_always(_gridData.update(_vertData));
	core_assert_always(_gridData.update(_fragData));

	const bool wasActive = _gridShader.isActive();
	if (!wasActive) {
		_gridShader.activate();
	}
	core_assert_always(_gridShader.setVert(_gridData.getVertUniformBuffer()));
	core_assert_always(_gridShader.setFrag(_gridData.getFragUniformBuffer()));
	core_assert_always(vbo.bind());
	video::drawElements<video::ShapeBuilder::Indices::value_type>(video::Primitive::Triangles, indices);
	vbo.unbind();
	if (!wasActive) {
		_gridShader.deactivate();
	}
}

void GridRenderer::createPlane() {
	if (_planeGridSize < 1) {
		return;
	}
	_shapeBuilder.clear();
	const float s = (float)_planeGridSize;
	_shapeBuilder.cube(glm::vec3(-s, 0.0f, -s), glm::vec3(s, 0.0f, s), video::ShapeBuilderCube::Bottom);
	uploadMesh(_planeVbo, _planeVertexIndex, _planeIndexIndex, _shapeBuilder);
}

void GridRenderer::update(const math::AABB<float> &aabb) {
	if (!aabb.isValid()) {
		return;
	}
	if (!_dirty && _aabb == aabb) {
		return;
	}
	if (_resolution.x <= 0 || _resolution.y <= 0 || _resolution.z <= 0) {
		return;
	}
	_aabb = aabb;
	_shapeBuilder.clear();
	const glm::vec3 pad(0.02f);
	_shapeBuilder.cube(aabb.mins() - pad, aabb.maxs() + pad, video::ShapeBuilderCube::All);
	uploadMesh(_gridVbo, _gridVertexIndex, _gridIndexIndex, _shapeBuilder);

	createForwardArrow(aabb);

	_dirty = false;
}

void GridRenderer::clear() {
	_shapeBuilder.clear();
	_dirty = false;
}

void GridRenderer::render(const video::Camera &camera, const math::AABB<float> &aabb, const glm::mat4 &model) {
	core_trace_scoped(GridRendererRender);

	if (_dirty) {
		update(aabb);
	}

	if ((!_renderGrid && !_renderAABB) || !aabb.isValid() || _gridVertexIndex < 0) {
		return;
	}

	video::ScopedBlendMode blend(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha,
								 video::BlendEquation::Add);
	video::ScopedState depthMask(video::State::DepthMask, false);
	video::ScopedFaceCull cull(video::Face::Front);
	if (_renderGrid) {
		const glm::vec3 step((float)_resolution.x, (float)_resolution.y, (float)_resolution.z);
		drawMesh(_gridVbo, _gridVertexIndex, _gridIndexIndex, camera, model, aabb.mins(), step, 5.0f);
	} else {
		const glm::vec3 extent = glm::max(aabb.maxs() - aabb.mins(), glm::vec3(1.0e-4f));
		drawMesh(_gridVbo, _gridVertexIndex, _gridIndexIndex, camera, model, aabb.mins(), extent, 1.0f);
	}
}

void GridRenderer::renderPlane(const video::Camera &camera, const glm::mat4 &model) {
	if (!_renderPlane) {
		return;
	}
	if (_dirtyPlane) {
		createPlane();
		_dirtyPlane = false;
	}
	if (_planeVertexIndex < 0) {
		return;
	}
	video::ScopedBlendMode blend(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha,
								 video::BlendEquation::Add);
	video::ScopedState depthMask(video::State::DepthMask, false);
	video::ScopedState cullFace(video::State::CullFace, false);
	const float s = (float)_planeGridSize;
	const glm::vec3 mins(-s, 0.0f, -s);
	const glm::vec3 step(1.0f, 1.0f, 1.0f);
	drawMesh(_planeVbo, _planeVertexIndex, _planeIndexIndex, camera, model, mins, step, 10.0f);
}

void GridRenderer::renderForwardArrow(const video::Camera &camera, const glm::mat4 &model) {
	_shapeRenderer.hide(_arrow, false);
	video::ScopedState cull(video::State::CullFace, false);
	_shapeRenderer.render(_arrow, camera, model);
	_shapeRenderer.hide(_arrow, true);
}

void GridRenderer::shutdown() {
	_arrow = -1;
	_gridVertexIndex = -1;
	_gridIndexIndex = -1;
	_planeVertexIndex = -1;
	_planeIndexIndex = -1;
	_gridVbo.shutdown();
	_planeVbo.shutdown();
	_gridData.shutdown();
	_gridShader.shutdown();
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

} // namespace render
