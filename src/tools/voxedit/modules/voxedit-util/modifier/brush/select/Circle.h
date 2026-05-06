/**
 * @file
 */

#pragma once

#include "SelectStrategy.h"
#include "core/collection/DynamicArray.h"
#include "voxel/Face.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace voxedit {
namespace select {

class Circle : public Strategy {
private:
	bool _previewMode = false;
	glm::ivec3 _ellipseCenter{0};
	int _ellipseRadiusU = 0;
	int _ellipseRadiusV = 0;
	int _ellipseDepth = 1;
	bool _ellipse3D = false;
	voxel::FaceNames _ellipseFace = voxel::FaceNames::Max;
	bool _ellipseValid = false;
	core::DynamicArray<glm::ivec3> _ellipseHistory;

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
	const glm::ivec3 &center() const {
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
	core::DynamicArray<glm::ivec3> &history() {
		return _ellipseHistory;
	}

	void setCenter(const glm::ivec3 &center) {
		_ellipseCenter = center;
	}
	void setRadiusU(int r) {
		_ellipseRadiusU = glm::max(r, 0);
	}
	void setRadiusV(int r) {
		_ellipseRadiusV = glm::max(r, 0);
	}
	void setDepth(int d) {
		_ellipseDepth = glm::max(d, 1);
	}
	void set3D(bool v) {
		_ellipse3D = v;
	}
	void setPreviewMode(bool v) {
		_previewMode = v;
	}

	static void ellipseAxes(voxel::FaceNames face, int &uAxis, int &vAxis);
	static bool insideEllipse(const glm::ivec3 &pos, const glm::ivec3 &center, int radiusU, int radiusV, int uAxis,
							  int vAxis);
	static bool insideSelection(const glm::ivec3 &pos, const glm::ivec3 &center, int radiusU, int radiusV, int depth,
								bool is3D, int uAxis, int vAxis, int faceAxisIdx, bool positiveNormal);
};

} // namespace select
} // namespace voxedit
