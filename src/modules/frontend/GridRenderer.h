#pragma once

#include "video/Camera.h"
#include "frontend/ShapeRenderer.h"
#include "video/ShapeBuilder.h"
#include "voxel/polyvox/Region.h"

namespace frontend {

class GridRenderer {
protected:
	video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;

	int32_t _aabbMeshIndex = -1;
	int32_t _gridMeshIndexXYNear = -1;
	int32_t _gridMeshIndexXYFar = -1;
	int32_t _gridMeshIndexXZNear = -1;
	int32_t _gridMeshIndexXZFar = -1;
	int32_t _gridMeshIndexYZNear = -1;
	int32_t _gridMeshIndexYZFar = -1;

	bool _renderAABB;
	bool _renderGrid;
public:
	GridRenderer(bool renderAABB = true, bool renderGrid = true);

	void render(const video::Camera& camera, const voxel::Region& region);

	bool renderAABB() const;
	void setRenderAABB(bool renderAABB);

	bool renderGrid() const;
	void setRenderGrid(bool renderGrid);

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
