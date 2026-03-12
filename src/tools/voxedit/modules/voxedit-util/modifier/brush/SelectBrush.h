/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "color/ColorUtil.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace voxedit {

class SceneManager;

/**
 * @brief Selection mode for the SelectBrush
 */
enum class SelectMode : uint8_t {
	All,
	/** Select only visible surface voxels in the AABB region */
	Surface,
	/** Select only voxels with the same color as the clicked voxel */
	SameColor,
	/** Select only voxels with the similar color as the clicked voxel */
	FuzzyColor,
	/** Select voxels connected to the clicked voxel with the same color (flood fill) */
	Connected,
	/** Flood-fill select all connected solid voxels that share the same exposed face as the clicked voxel */
	FlatSurface,
	/** Replace the current selection with all solid voxels in the drawn 3D box region */
	Box3D,
	/** Select surface voxels within a circular radius from the clicked center point */
	Circle,
	/** Flood-fill select connected surface voxels that fit a global slope plane within a height deviation threshold */
	Slope,

	Max
};

/**
 * @ingroup Brushes
 */
class SelectBrush : public AABBBrush {
private:
	using Super = AABBBrush;
	SelectMode _selectMode = SelectMode::All;
	float _colorThreshold = color::ApproximationDistanceModerate;
	int _flatDeviation = 0;
	int _slopeDeviation = 10;
	int _slopeSampleDistance = 3;
	bool _previewMode = false;

	/** Cached ellipse parameters from the last Circle selection */
	glm::ivec3 _ellipseCenter{0};
	int _ellipseRadiusU = 0;
	int _ellipseRadiusV = 0;
	int _ellipseDepth = 1;
	bool _ellipse3D = false;
	voxel::FaceNames _ellipseFace = voxel::FaceNames::Max;
	bool _ellipseValid = false;

	/** Positions flagged by the current ellipse -used to undo only our flags on slider changes */
	core::DynamicArray<glm::ivec3> _ellipseHistory;

	/** Cached slope parameters from the last Slope selection */
	glm::ivec3 _slopeSeedPos{0};
	voxel::FaceNames _slopeFace = voxel::FaceNames::Max;
	bool _slopeValid = false;

	/** Positions flagged by the current slope selection -used to undo on slider changes */
	core::DynamicArray<glm::ivec3> _slopeHistory;

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	SelectBrush()
		: Super(BrushType::Select, ModifierType::Override, ModifierType::Override | ModifierType::Erase) {
		setBrushClamping(true);
	}
	virtual ~SelectBrush() = default;

	voxel::Region calcRegion(const BrushContext &ctx) const override;
	bool managesOwnSelection() const override {
		return true;
	}
	bool needsAdditionalAction(const BrushContext &ctx) const override;
	bool beginBrush(const BrushContext &ctx) override;

	void setSelectMode(SelectMode mode) {
		if (_selectMode != mode) {
			_ellipseValid = false;
			_slopeValid = false;
		}
		_selectMode = mode;
	}

	SelectMode selectMode() const {
		return _selectMode;
	}

	void setColorThreshold(float threshold) {
		_colorThreshold = threshold;
	}

	float colorThreshold() const {
		return _colorThreshold;
	}

	static constexpr int MaxFlatDeviation = 32;

	void setFlatDeviation(int deviation) {
		_flatDeviation = glm::clamp(deviation, 0, MaxFlatDeviation);
	}

	int flatDeviation() const {
		return _flatDeviation;
	}

	static constexpr int MaxSlopeDeviation = 90;

	void setSlopeDeviation(int angle) {
		_slopeDeviation = glm::clamp(angle, 0, MaxSlopeDeviation);
	}

	int slopeDeviation() const {
		return _slopeDeviation;
	}

	static constexpr int MinSlopeSampleDistance = 2;
	static constexpr int MaxSlopeSampleDistance = 16;

	void setSlopeSampleDistance(int dist) {
		_slopeSampleDistance = glm::clamp(dist, MinSlopeSampleDistance, MaxSlopeSampleDistance);
	}

	int slopeSampleDistance() const {
		return _slopeSampleDistance;
	}

	void setPreviewMode(bool v) {
		_previewMode = v;
	}

	/** Get the perpendicular axis indices for a given face */
	static void ellipseAxes(voxel::FaceNames face, int &uAxis, int &vAxis);

	/** Check if a position is inside an ellipse defined on the face plane */
	static bool insideEllipse(const glm::ivec3 &pos, const glm::ivec3 &center,
							  int radiusU, int radiusV, int uAxis, int vAxis);

	/** Full bounds check: 2D ellipse + depth, or 3D ellipsoid (behind surface only) */
	static bool insideSelection(const glm::ivec3 &pos, const glm::ivec3 &center,
								int radiusU, int radiusV, int depth, bool is3D,
								int uAxis, int vAxis, int faceAxisIdx, bool positiveNormal);

	bool ellipseValid() const {
		return _ellipseValid;
	}

	const glm::ivec3 &ellipseCenter() const {
		return _ellipseCenter;
	}

	void setEllipseCenter(const glm::ivec3 &center) {
		_ellipseCenter = center;
	}

	int ellipseRadiusU() const {
		return _ellipseRadiusU;
	}

	void setEllipseRadiusU(int r) {
		_ellipseRadiusU = glm::max(r, 0);
	}

	int ellipseRadiusV() const {
		return _ellipseRadiusV;
	}

	void setEllipseRadiusV(int r) {
		_ellipseRadiusV = glm::max(r, 0);
	}

	int ellipseDepth() const {
		return _ellipseDepth;
	}

	void setEllipseDepth(int d) {
		_ellipseDepth = glm::max(d, 1);
	}

	bool ellipse3D() const {
		return _ellipse3D;
	}

	void setEllipse3D(bool v) {
		_ellipse3D = v;
	}

	voxel::FaceNames ellipseFace() const {
		return _ellipseFace;
	}

	void invalidateEllipse() {
		_ellipseValid = false;
		_ellipseHistory.clear();
	}

	core::DynamicArray<glm::ivec3> &ellipseHistory() {
		return _ellipseHistory;
	}

	bool slopeValid() const {
		return _slopeValid;
	}

	const glm::ivec3 &slopeSeedPos() const {
		return _slopeSeedPos;
	}

	voxel::FaceNames slopeFace() const {
		return _slopeFace;
	}

	void invalidateSlope() {
		_slopeValid = false;
		_slopeHistory.clear();
	}

	core::DynamicArray<glm::ivec3> &slopeHistory() {
		return _slopeHistory;
	}
};

} // namespace voxedit
