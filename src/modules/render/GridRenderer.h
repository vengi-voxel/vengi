/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "render/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "voxel/polyvox/Region.h"
#include "core/IComponent.h"

namespace render {

/**
 * @brief Renders a grid or bounding box for a given region
 *
 * @note Also hides sides of the grid that would occlude the view to the inside
 */
class GridRenderer {
protected:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;

	int32_t _aabbMeshIndex = -1;
	int32_t _gridMeshIndexXYNear = -1;
	int32_t _gridMeshIndexXYFar = -1;
	int32_t _gridMeshIndexXZNear = -1;
	int32_t _gridMeshIndexXZFar = -1;
	int32_t _gridMeshIndexYZNear = -1;
	int32_t _gridMeshIndexYZFar = -1;

	int _resolution = 1;
	bool _renderAABB;
	bool _renderGrid;
	bool _dirty = false;
public:
	GridRenderer(bool renderAABB = true, bool renderGrid = true);

	bool setGridResolution(int resolution);
	int gridResolution() const;

	/**
	 * @param region The region to do the plane culling with
	 */
	void render(const video::Camera& camera, const voxel::Region& region);

	bool renderAABB() const;
	void setRenderAABB(bool renderAABB);

	bool renderGrid() const;
	void setRenderGrid(bool renderGrid);

	/**
	 * @brief Update the internal render buffers for the new region.
	 * @param region The region to render the grid for
	 */
	void update(const voxel::Region& region);
	void clear();

	/**
	 * @sa shutdown()
	 */
	bool init();

	void shutdown();
};

inline bool GridRenderer::renderAABB() const {
	return _renderAABB;
}

inline bool GridRenderer::renderGrid() const {
	return _renderGrid;
}

inline void GridRenderer::setRenderAABB(bool renderAABB) {
	_renderAABB = renderAABB;
}

inline void GridRenderer::setRenderGrid(bool renderGrid) {
	_renderGrid = renderGrid;
}

}
