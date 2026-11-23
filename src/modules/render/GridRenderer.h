/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "math/AABB.h"
#include "render/ShapeRenderer.h"
#include "video/Buffer.h"
#include "video/ShapeBuilder.h"
#include "GridShader.h"
#include "GridData.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace video {
class Camera;
}

namespace render {

/**
 * @brief Renders a grid or bounding box for a given region
 *
 * Volume faces and the ground plane use a screen-space fwidth grid overlay so
 * lines stay about one pixel wide. Far box faces are shown via front-face culling.
 */
class GridRenderer {
protected:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	math::AABB<float> _aabb;
	math::Vec4f _color{1.0f, 1.0f, 1.0f, 1.0f};

	shader::GridShader &_gridShader;
	shader::GridData _gridData;
	alignas(16) shader::GridData::VertData _vertData{};
	alignas(16) shader::GridData::FragData _fragData{};
	video::Buffer _gridVbo;
	video::Buffer _planeVbo;
	int32_t _gridVertexIndex = -1;
	int32_t _gridIndexIndex = -1;
	int32_t _planeVertexIndex = -1;
	int32_t _planeIndexIndex = -1;

	int32_t _arrow = -1;
	void createForwardArrow(const math::AABB<float> &aabb);
	void createPlane();
	bool uploadMesh(video::Buffer &vbo, int32_t &vertexIndex, int32_t &indexIndex, const video::ShapeBuilder &builder);
	void drawMesh(video::Buffer &vbo, int32_t vertexIndex, int32_t indexIndex, const video::Camera &camera,
				  const math::Mat4f &model, const math::Vec3f &mins, const math::Vec3f &step, float majorStride);
	int _planeGridSize = -1;
	math::Vec3i _resolution{-1};
	bool _renderAABB;
	bool _renderGrid;
	bool _renderPlane;
	bool _dirty = false;
	bool _dirtyPlane = true;
public:
	GridRenderer(bool renderAABB = false, bool renderGrid = true, bool renderPlane = false);

	bool setGridResolution(const math::Vec3i &resolution);
	bool setGridResolution(int resolution);
	const math::Vec3i &gridResolution() const;

	/**
	 * @param aabb The region to do the plane culling with
	 */
	void render(const video::Camera &camera, const math::AABB<float> &aabb, const math::Mat4f &model = math::Mat4f(1.0f));
	void renderForwardArrow(const video::Camera &camera, const math::Mat4f &model = math::Mat4f(1.0f));
	void renderPlane(const video::Camera &camera, const math::Mat4f &model = math::Mat4f(1.0f));

	void setPlaneGridSize(int planeGridSize);
	int planeGridSize() const;

	bool isRenderAABB() const;
	void setRenderAABB(bool renderAABB);

	bool isRenderPlane() const;
	void setRenderPlane(bool renderPlane);

	bool isRenderGrid() const;
	void setRenderGrid(bool renderGrid);

	/**
	 * @brief Update the internal render buffers for the new region.
	 * @param region The region to render the grid for
	 */
	void update(const math::AABB<float> &region);
	void clear();
	void setColor(const math::Vec4f &color);

	/**
	 * @sa shutdown()
	 */
	bool init();

	void shutdown();
};

inline void GridRenderer::setPlaneGridSize(int planeGridSize) {
	if (_planeGridSize == planeGridSize) {
		return;
	}
	_planeGridSize = planeGridSize;
	_dirtyPlane = true;
}

inline int GridRenderer::planeGridSize() const {
	return _planeGridSize;
}

inline bool GridRenderer::isRenderPlane() const {
	return _renderPlane;
}

inline void GridRenderer::setRenderPlane(bool renderPlane) {
	if (_renderPlane == renderPlane) {
		return;
	}
	_renderPlane = renderPlane;
}

inline bool GridRenderer::isRenderAABB() const {
	return _renderAABB;
}

inline bool GridRenderer::isRenderGrid() const {
	return _renderGrid;
}

inline void GridRenderer::setRenderAABB(bool renderAABB) {
	_renderAABB = renderAABB;
}

inline void GridRenderer::setRenderGrid(bool renderGrid) {
	if (_renderGrid == renderGrid) {
		return;
	}
	_renderGrid = renderGrid;
}

} // namespace render
