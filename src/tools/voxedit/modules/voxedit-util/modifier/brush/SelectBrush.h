/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "color/Distance.h"
#include "core/collection/DynamicArray.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/DynamicVoxelArray.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace voxel {
class RawVolume;
} // namespace voxel

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
	/** Free-form polygon selection: click vertices to build a polygon, close it to select enclosed surface voxels */
	Lasso,
	/** Continuous paint-style selection: hold mouse and drag to select solid voxels within brush radius.
	 *  Uses single mode for continuous execution. Single undo entry on release. */
	Paint,
	/** Selection mode driven by a user-supplied Lua script */
	Script,
	Max
};

class LUASelectionMode;

/**
 * @ingroup Brushes
 */
class SelectBrush : public AABBBrush {
public:
	/** Max U/V distance from the first vertex that auto-closes the polygon on click */
	static constexpr int LassoCloseThresholdVoxels = 1;
	/** Initial capacity reserved for the lasso polygon vertex list */
	static constexpr int LassoPathInitialReserve = 32;
	/** Sentinel value for _lastLassoCursorPos to force a refresh on first update() */
	static constexpr int LassoInvalidCursorCoord = -100000;

private:
	using Super = AABBBrush;
	SceneManager *_sceneManager = nullptr;
	SelectMode _selectMode = SelectMode::All;
	/** Index into the Modifier's lua selection mode array, or -1 for native mode */
	int _luaSelectionModeIndex = -1;
	/** Pointer to the active lua selection mode (set by Modifier when index >= 0) */
	LUASelectionMode *_activeLuaSelectionMode = nullptr;
	float _colorThreshold = color::ApproximationDistanceModerate;
	int _flatDeviation = 0;
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

	/** Lasso polygon vertices accumulated across multiple clicks */
	core::DynamicArray<glm::ivec3> _lassoPath;
	/** Original voxels at edge mark positions - used to revert edge flags on cancel */
	voxel::DynamicVoxelArray _lassoEdgeHistory;
	/** True while the user is still building the lasso polygon (not yet closed) */
	bool _lassoAccumulating = false;
	/** U and V axis indices for projection onto the clicked face plane */
	int _lassoUAxis = 0;
	int _lassoVAxis = 1;
	/** Face normal axis index */
	int _lassoFaceAxisIdx = 2;
	/** Face from the first lasso click, locked for the entire polygon */
	voxel::FaceNames _lassoFace = voxel::FaceNames::Max;
	/** Last cursor position seen by update(), used to detect movement between clicks */
	glm::ivec3 _lastLassoCursorPos{LassoInvalidCursorCoord};

	/** True while paint-select drag is active (between beginBrush and endBrush) */
	bool _paintAccumulating = false;
	/** When true, paint-select only adds voxels adjacent to already-selected voxels */
	bool _paintGrowRegion = false;
	/** True if any selection existed when the current paint drag started */
	bool _paintHadSelection = false;
	/** Accumulated dirty region across all paint-select ticks */
	voxel::Region _paintDirtyRegion = voxel::Region::InvalidRegion;
	/** Saved dirty region from endBrush, consumed by consumePendingUndoRegion */
	voxel::Region _paintFinalUndoRegion = voxel::Region::InvalidRegion;

	/** Box3D selection region cached for ModifierVolumeWrapper to allow editing air voxels inside the box */
	voxel::Region _box3DSelectionRegion = voxel::Region::InvalidRegion;

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	SelectBrush(SceneManager *sceneManager) : Super(BrushType::Select, ModifierType::Override, ModifierType::Override | ModifierType::Erase), _sceneManager(sceneManager) {
		setBrushClamping(true);
	}
	virtual ~SelectBrush() = default;

	voxel::Region calcRegion(const BrushContext &ctx) const override;
	bool managesOwnSelection() const override;
	bool active() const override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	bool needsAdditionalAction(const BrushContext &ctx) const override;
	bool beginBrush(const BrushContext &ctx) override;
	void reset() override;
	void onSceneChange() override;

	void setSelectMode(SelectMode mode);
	SelectMode selectMode() const;

	/** Set the active lua selection mode. Pass -1 to use native C++ modes. */
	void setLuaSelectionMode(int index, LUASelectionMode *mode);
	/** Get the active lua selection mode index (-1 = native mode) */
	int luaSelectionModeIndex() const;
	/** Get the active lua selection mode (nullptr if native mode) */
	LUASelectionMode *activeLuaSelectionMode() const;

	const voxel::Region &box3DSelectionRegion() const;
	void setBox3DSelectionRegion(const voxel::Region &region);

	void setColorThreshold(float threshold);
	float colorThreshold() const;

	static constexpr int MaxFlatDeviation = 32;
	void setFlatDeviation(int deviation);
	int flatDeviation() const;

	void setPreviewMode(bool v);

	bool lassoAccumulating() const;
	const core::DynamicArray<glm::ivec3> &lassoPath() const;

	void endBrush(BrushContext &ctx) override;
	bool hasPendingChanges() const override;
	voxel::Region revertChanges(voxel::RawVolume *volume) override;
	voxel::Region consumePendingUndoRegion() override;
	void abort(BrushContext &ctx) override;

	bool paintGrowRegion() const;
	void setPaintGrowRegion(bool v);
	int lassoUAxis() const;
	int lassoVAxis() const;

	/** Discard the in-progress lasso polygon (caller must clean up edge history voxels in the volume) */
	void invalidateLasso();
	/** Remove the last placed lasso vertex. Does not redraw edges - caller must call redrawEdgesOnVolume(). */
	void popLastLassoPathEntry();

	/**
	 * @brief Redraw all committed lasso edges directly on a raw volume.
	 *        Updates _lassoEdgeHistory with the new set of edge positions.
	 *        Caller is responsible for calling revertChanges() first to clear old marks.
	 * @param volume Target volume to write edge marks onto
	 * @param region Bounding region of the volume (used to clamp column scans)
	 * @param outDirty Accumulates the positions modified by this call
	 */
	void redrawEdgesOnVolume(voxel::RawVolume *volume, const voxel::Region &region, voxel::Region &outDirty);

	/** Get the perpendicular axis indices for a given face */
	static void ellipseAxes(voxel::FaceNames face, int &uAxis, int &vAxis);
	/** Check if a position is inside an ellipse defined on the face plane */
	static bool insideEllipse(const glm::ivec3 &pos, const glm::ivec3 &center, int radiusU, int radiusV, int uAxis,
							  int vAxis);
	/** Full bounds check: 2D ellipse + depth, or 3D ellipsoid (behind surface only) */
	static bool insideSelection(const glm::ivec3 &pos, const glm::ivec3 &center, int radiusU, int radiusV, int depth,
								bool is3D, int uAxis, int vAxis, int faceAxisIdx, bool positiveNormal);

	bool ellipseValid() const;
	const glm::ivec3 &ellipseCenter() const;
	void setEllipseCenter(const glm::ivec3 &center);
	int ellipseRadiusU() const;
	void setEllipseRadiusU(int r);
	int ellipseRadiusV() const;
	void setEllipseRadiusV(int r);
	int ellipseDepth() const;
	void setEllipseDepth(int d);
	bool ellipse3D() const;
	void setEllipse3D(bool v);
	voxel::FaceNames ellipseFace() const;
	void invalidateEllipse();
	core::DynamicArray<glm::ivec3> &ellipseHistory();

};

inline bool SelectBrush::managesOwnSelection() const {
	return true;
}

inline SelectMode SelectBrush::selectMode() const {
	return _selectMode;
}

inline int SelectBrush::luaSelectionModeIndex() const {
	return _luaSelectionModeIndex;
}

inline LUASelectionMode *SelectBrush::activeLuaSelectionMode() const {
	return _activeLuaSelectionMode;
}

inline const voxel::Region &SelectBrush::box3DSelectionRegion() const {
	return _box3DSelectionRegion;
}

inline void SelectBrush::setBox3DSelectionRegion(const voxel::Region &region) {
	_box3DSelectionRegion = region;
}

inline void SelectBrush::setColorThreshold(float threshold) {
	_colorThreshold = threshold;
}

inline float SelectBrush::colorThreshold() const {
	return _colorThreshold;
}

inline void SelectBrush::setFlatDeviation(int deviation) {
	_flatDeviation = glm::clamp(deviation, 0, MaxFlatDeviation);
}

inline int SelectBrush::flatDeviation() const {
	return _flatDeviation;
}

inline void SelectBrush::setPreviewMode(bool v) {
	_previewMode = v;
}

inline bool SelectBrush::lassoAccumulating() const {
	return _lassoAccumulating;
}

inline const core::DynamicArray<glm::ivec3> &SelectBrush::lassoPath() const {
	return _lassoPath;
}

inline bool SelectBrush::paintGrowRegion() const {
	return _paintGrowRegion;
}

inline void SelectBrush::setPaintGrowRegion(bool v) {
	_paintGrowRegion = v;
}

inline int SelectBrush::lassoUAxis() const {
	return _lassoUAxis;
}

inline int SelectBrush::lassoVAxis() const {
	return _lassoVAxis;
}

inline bool SelectBrush::ellipseValid() const {
	return _ellipseValid;
}

inline const glm::ivec3 &SelectBrush::ellipseCenter() const {
	return _ellipseCenter;
}

inline void SelectBrush::setEllipseCenter(const glm::ivec3 &center) {
	_ellipseCenter = center;
}

inline int SelectBrush::ellipseRadiusU() const {
	return _ellipseRadiusU;
}

inline void SelectBrush::setEllipseRadiusU(int r) {
	_ellipseRadiusU = glm::max(r, 0);
}

inline int SelectBrush::ellipseRadiusV() const {
	return _ellipseRadiusV;
}

inline void SelectBrush::setEllipseRadiusV(int r) {
	_ellipseRadiusV = glm::max(r, 0);
}

inline int SelectBrush::ellipseDepth() const {
	return _ellipseDepth;
}

inline void SelectBrush::setEllipseDepth(int d) {
	_ellipseDepth = glm::max(d, 1);
}

inline bool SelectBrush::ellipse3D() const {
	return _ellipse3D;
}

inline void SelectBrush::setEllipse3D(bool v) {
	_ellipse3D = v;
}

inline voxel::FaceNames SelectBrush::ellipseFace() const {
	return _ellipseFace;
}

inline core::DynamicArray<glm::ivec3> &SelectBrush::ellipseHistory() {
	return _ellipseHistory;
}

} // namespace voxedit
