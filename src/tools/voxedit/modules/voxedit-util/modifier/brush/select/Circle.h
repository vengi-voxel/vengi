/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Face.h"
#include "math/Vec3i.h"

namespace voxedit {
namespace select {

class Circle : public Strategy {
private:
	using Super = Strategy;
	math::Vec3i _ellipseCenter{0};
	int _ellipseRadiusU = 0;
	int _ellipseRadiusV = 0;
	int _ellipseDepth = 1;
	bool _ellipse3D = false;
	voxel::FaceNames _ellipseFace = voxel::FaceNames::Max;
	bool _ellipseValid = false;
	core::DynamicArray<math::Vec3i> _ellipseHistory;

public:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region, const AABBBrushState &state) override;
	voxel::Region calcRegion(const BrushContext &ctx, const AABBBrushState &state) const override;
	bool beginBrush(const BrushContext &ctx, const AABBBrushState &state) override;
	void reset() override;

	void invalidate();

	bool valid() const {
		return _ellipseValid;
	}

	const math::Vec3i &center() const {
		return _ellipseCenter;
	}

	int radiusU() const {
		return _ellipseRadiusU;
	}

	int radiusV() const {
		return _ellipseRadiusV;
	}

	int depth() const {
		return _ellipseDepth;
	}

	bool is3D() const {
		return _ellipse3D;
	}

	voxel::FaceNames face() const {
		return _ellipseFace;
	}

	core::DynamicArray<math::Vec3i> &history() {
		return _ellipseHistory;
	}

	void setCenter(const math::Vec3i &center) {
		_ellipseCenter = center;
	}

	void setRadiusU(int r) {
		_ellipseRadiusU = math::max(r, 0);
	}

	void setRadiusV(int r) {
		_ellipseRadiusV = math::max(r, 0);
	}

	void setDepth(int d) {
		_ellipseDepth = math::max(d, 1);
	}

	void set3D(bool v) {
		_ellipse3D = v;
	}

	static void ellipseAxes(voxel::FaceNames face, int &uAxis, int &vAxis);
	static bool insideEllipse(const math::Vec3i &pos, const math::Vec3i &center, int radiusU, int radiusV, int uAxis,
							  int vAxis);
	static bool insideSelection(const math::Vec3i &pos, const math::Vec3i &center, int radiusU, int radiusV, int depth,
								bool is3D, int uAxis, int vAxis, int faceAxisIdx, bool positiveNormal);
};

} // namespace select
} // namespace voxedit
