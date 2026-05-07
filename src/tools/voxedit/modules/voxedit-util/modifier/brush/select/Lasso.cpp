/**
 * @file
 */

#include "Lasso.h"
#include "app/ForParallel.h"
#include "core/collection/DynamicMap.h"
#include "math/Ray.h"
#include "video/Camera.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/BrushGizmo.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/Raycast.h"
#include <glm/geometric.hpp>

namespace voxedit {
namespace select {

bool Lasso::wantBrushGizmo(const BrushContext &ctx) const {
	return _screenDragging && _screenPoints.size() >= 2;
}

void Lasso::brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const {
	state.operations = BrushGizmo_ScreenPolygon;
	state.screenPolygon = &_screenPoints;
}

bool Lasso::active() const {
	return _screenDragging;
}

void Lasso::reset() {
	_screenDragging = false;
	_screenPoints.clear();
}

void Lasso::abort(BrushContext &ctx) {
	if (_screenDragging) {
		_screenDragging = false;
		_screenPoints.clear();
		_modifiedFlags = SceneModifiedFlags::All;
	}
}

void Lasso::endBrush(BrushContext &ctx) {
	_screenDragging = false;
	_screenPoints.clear();
}

void Lasso::update(const BrushContext &ctx, double nowSeconds) {
	if (!_screenDragging || !_sceneManager) {
		return;
	}
	const glm::ivec2 &mouse = _sceneManager->mousePos();
	const glm::vec2 pt((float)mouse.x, (float)mouse.y);
	if (_screenPoints.empty() || glm::distance(_screenPoints.back(), pt) > 2.0f) {
		_screenPoints.push_back(pt);
	}
}

bool Lasso::beginBrush(const BrushContext &ctx, const AABBBrushState &state) {
	if (_screenDragging) {
		return true;
	}
	_screenPoints.clear();
	_screenDragging = true;
	_modifiedFlags = SceneModifiedFlags::NoUndo;
	if (_sceneManager) {
		const glm::ivec2 &mouse = _sceneManager->mousePos();
		_screenPoints.push_back(glm::vec2((float)mouse.x, (float)mouse.y));
	}
	return true;
}

void Lasso::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
					 const voxel::Region &region, const AABBBrushState &state) {
	if (!_screenDragging || _screenPoints.size() < 3 || !_sceneManager) {
		return;
	}
	const video::Camera *camera = _sceneManager->activeCamera();
	if (!camera) {
		_screenDragging = false;
		_screenPoints.clear();
		return;
	}
	_modifiedFlags = SceneModifiedFlags::All;
	_screenDragging = false;

	const glm::mat4 invModel = glm::inverse(_sceneManager->worldMatrix());

	glm::vec2 bmin = _screenPoints[0];
	glm::vec2 bmax = _screenPoints[0];
	for (const glm::vec2 &pt : _screenPoints) {
		bmin = glm::min(bmin, pt);
		bmax = glm::max(bmax, pt);
	}
	const int y0 = glm::max((int)bmin.y, 0);
	const int y1 = glm::min((int)bmax.y, camera->size().y - 1);
	const int screenMaxX = camera->size().x - 1;

	const float rayLength = camera->farPlane();
	const int polySize = (int)_screenPoints.size();

	static constexpr int stride = 2;
	const int rowCount = (y1 - y0) / stride + 1;

	if (rowCount <= 0) {
		_screenPoints.clear();
		return;
	}

	// Parallel phase: each chunk collects hit positions into its own array
	const int chunkCount = app::for_parallel_size(0, rowCount);
	core::DynamicArray<core::DynamicArray<glm::ivec3>> chunkResults(chunkCount);

	const auto &screenPoints = _screenPoints;
	voxel::RawVolume *volume = wrapper.volume();

	app::for_parallel(0, rowCount, [&](int startIdx, int endIdx) {
		// Determine which chunk this is for result storage
		const int chunkSize = (rowCount + chunkCount - 1) / chunkCount;
		const int chunkIdx = startIdx / chunkSize;
		core::DynamicArray<glm::ivec3> &results = chunkResults[chunkIdx];
		core::DynamicArray<float> intersections;
		intersections.reserve(16);
		core::DynamicMap<glm::ivec3, bool, 1031, glm::hash<glm::ivec3>> visited;

		for (int rowIdx = startIdx; rowIdx < endIdx; ++rowIdx) {
			const int y = y0 + rowIdx * stride;
			if (y > y1) {
				break;
			}
			const float fy = (float)y;

			// Scanline: compute x-intersections of polygon edges with this row
			intersections.clear();
			for (int i = 0, j = polySize - 1; i < polySize; j = i++) {
				const glm::vec2 &vi = screenPoints[i];
				const glm::vec2 &vj = screenPoints[j];
				if ((vi.y > fy) != (vj.y > fy)) {
					const float xInt = vi.x + (fy - vi.y) / (vj.y - vi.y) * (vj.x - vi.x);
					intersections.push_back(xInt);
				}
			}
			if (intersections.size() < 2) {
				continue;
			}
			core::sort(intersections.begin(), intersections.end(), [](float a, float b) { return a < b; });

			// Process pairs of intersections (even-odd fill rule)
			for (int p = 0; p + 1 < (int)intersections.size(); p += 2) {
				int xStart = glm::max((int)ceilf(intersections[p]), 0);
				int xEnd = glm::min((int)floorf(intersections[p + 1]), screenMaxX);
				xStart = ((xStart + stride - 1) / stride) * stride;

				for (int x = xStart; x <= xEnd; x += stride) {
					const math::Ray worldRay = camera->mouseRay(glm::ivec2(x, y));
					const glm::vec3 origin = glm::vec3(invModel * glm::vec4(worldRay.origin, 1.0f));
					const glm::vec3 dir =
						glm::normalize(glm::vec3(invModel * glm::vec4(worldRay.direction, 0.0f)));
					const glm::vec3 end = origin + dir * rayLength;
					voxelutil::raycastWithEndpoints(
						volume, origin, end, [&](const voxel::RawVolume::Sampler &sampler) {
							const voxel::Voxel &voxel = sampler.voxel();
							if (voxel::isAir(voxel.getMaterial())) {
								return true;
							}
							const glm::ivec3 &pos = sampler.position();
							if (visited.putIfAbsent(pos, true)) {
								results.push_back(pos);
							}
							return false;
						});
				}
			}
		}
	});

	// Sequential phase: apply flags from collected positions
	const bool erase = wrapper.modifierType() == ModifierType::Erase;
	for (const auto &results : chunkResults) {
		for (const glm::ivec3 &pos : results) {
			if (erase) {
				wrapper.removeFlagAt(pos.x, pos.y, pos.z, voxel::FlagOutline);
			} else {
				wrapper.setFlagAt(pos.x, pos.y, pos.z, voxel::FlagOutline);
			}
		}
	}
	_screenPoints.clear();
}

} // namespace select
} // namespace voxedit
