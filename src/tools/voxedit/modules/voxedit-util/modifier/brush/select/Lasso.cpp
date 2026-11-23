/**
 * @file
 */

#include "Lasso.h"
#include "app/ForParallel.h"
#include "core/collection/DynamicMap.h"
#include "math/Ray.h"
#include "video/Camera.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/Brush.h"
#include "voxedit-util/modifier/brush/BrushGizmo.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/Raycast.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"
#include "math/Vec2f.h"
#include "math/Mat4Ops.h"

namespace voxedit {
namespace select {

bool Lasso::hasHandle(const BrushContext &ctx, const AABBBrushState &brushState) const {
	return _screenDragging && _screenPoints.size() >= 2;
}

void Lasso::describeHandle(const BrushContext &ctx, const AABBBrushState &brushState, BrushGizmoState &state) const {
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
	const math::Vec2i &mouse = _sceneManager->mousePos();
	const math::Vec2f pt((float)mouse.x, (float)mouse.y);
	if (_screenPoints.empty() || math::distance(_screenPoints.back(), pt) > 2.0f) {
		_screenPoints.push_back(pt);
	}
}

bool Lasso::beginBrush(const BrushContext &ctx, const AABBBrushState &state) {
	if (_screenDragging) {
		return true;
	}
	_screenPoints.clear();
	_screenDragging = true;
	if (_sceneManager) {
		const math::Vec2i &mouse = _sceneManager->mousePos();
		_screenPoints.push_back(math::Vec2f((float)mouse.x, (float)mouse.y));
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
	_screenDragging = false;

	const math::Mat4f invModel = _sceneManager->modifier().isSliceMode() ? math::Mat4f(1.0f) : math::inverse(_sceneManager->worldMatrix());

	math::Vec2f bmin = _screenPoints[0];
	math::Vec2f bmax = _screenPoints[0];
	for (const math::Vec2f &pt : _screenPoints) {
		bmin = math::min(bmin, pt);
		bmax = math::max(bmax, pt);
	}
	const int y0 = math::max((int)bmin.y, 0);
	const int y1 = math::min((int)bmax.y, camera->size().y - 1);
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
	core::DynamicArray<core::DynamicArray<math::Vec3i>> chunkResults(chunkCount);

	const auto &screenPoints = _screenPoints;
	voxel::RawVolume *volume = wrapper.volume();

	app::for_parallel(0, rowCount, [&](int startIdx, int endIdx) {
		// Determine which chunk this is for result storage
		const int chunkSize = (rowCount + chunkCount - 1) / chunkCount;
		const int chunkIdx = startIdx / chunkSize;
		core::DynamicArray<math::Vec3i> &results = chunkResults[chunkIdx];
		core::DynamicArray<float> intersections;
		intersections.reserve(16);
		core::DynamicMap<math::Vec3i, bool, 1031, math::Vec3iHash> visited;

		for (int rowIdx = startIdx; rowIdx < endIdx; ++rowIdx) {
			const int y = y0 + rowIdx * stride;
			if (y > y1) {
				break;
			}
			const float fy = (float)y;

			// Scanline: compute x-intersections of polygon edges with this row
			intersections.clear();
			for (int i = 0, j = polySize - 1; i < polySize; j = i++) {
				const math::Vec2f &vi = screenPoints[i];
				const math::Vec2f &vj = screenPoints[j];
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
				int xStart = math::max((int)math::ceil(intersections[p]), 0);
				int xEnd = math::min((int)math::floor(intersections[p + 1]), screenMaxX);
				xStart = ((xStart + stride - 1) / stride) * stride;

				for (int x = xStart; x <= xEnd; x += stride) {
					const math::Ray worldRay = camera->mouseRay(math::Vec2i(x, y));
					const math::Vec3f origin = math::Vec3f(invModel * math::Vec4f(worldRay.origin, 1.0f));
					const math::Vec3f dir = math::normalize(math::Vec3f(invModel * math::Vec4f(worldRay.direction, 0.0f)));
					const math::Vec3f end = origin + dir * rayLength;
					voxelutil::raycastWithEndpoints(volume, origin, end, [&](const voxel::RawVolume::Sampler &sampler) {
						const voxel::Voxel &voxel = sampler.voxel();
						if (voxel::isAir(voxel.getMaterial())) {
							return true;
						}
						const math::Vec3i &pos = sampler.position();
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
		for (const math::Vec3i &pos : results) {
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
