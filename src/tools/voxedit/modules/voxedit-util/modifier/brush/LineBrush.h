/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "LineState.h"
#include "core/collection/BitSet.h"
#include "core/collection/DynamicArray.h"
#include "math/Bezier.h"
#include "voxel/Region.h"

namespace voxedit {

/**
 * @brief Type alias for line stipple patterns (bit pattern for dashed lines)
 */
using LineStipplePattern = core::BitSet<9>;

/**
 * @brief Draws straight lines between two points with optional stippling
 *
 * The LineBrush uses raycasting to place voxels along a straight line from the
 * reference position to the cursor position. It supports:
 *
 * - **Stipple Patterns**: Create dashed or dotted lines using a bit pattern
 * - **Continuous Mode**: Chain multiple line segments without releasing the action button
 *
 * # Usage
 *
 * 1. Set the reference position (typically when beginning the brush action)
 * 2. Move cursor to the end position
 * 3. Execute to draw the line
 * 4. In continuous mode, the end point becomes the new reference for the next segment
 *
 * The stipple pattern determines which voxels are placed: each bit represents one
 * step along the line. A value of true places a voxel, false skips it.
 *
 * @ingroup Brushes
 * @sa LineState for tracking state changes
 */
class LineBrush : public Brush {
private:
	using Super = Brush;

public:
	using BezierSegment = math::BezierSegment;

protected:
	/** Cached state for detecting changes requiring preview update */
	LineState _state;
	/** Finalized preview-only bezier segments */
	core::DynamicArray<BezierSegment> _segments;
	/** Volume passed into preExecute(); used to distinguish preview from commit */
	const voxel::RawVolume *_lastVolume = nullptr;
	/** Currently selected pending bezier segment for gizmo editing */
	int _selectedSegment = -1;
	/** True between beginBrush() and endBrush() */
	bool _active = false;
	/** Set while auto-commit calls execute() during brush deactivation */
	bool _commitPending = false;
	/** If true, end position becomes next reference position */
	bool _continuous = false;
	/** If true, draw a quadratic bezier instead of a straight line */
	bool _bezier = false;
	/** True once the control point was moved explicitly */
	bool _hasCustomControlPoint = false;
	/** Quadratic bezier control point */
	glm::ivec3 _controlPoint{0};
	/** Line thickness in voxels (1 = single voxel) */
	int _thickness = 1;
	/** 9-bit pattern controlling which voxels are placed */
	LineStipplePattern _stipplePattern;

	glm::ivec3 defaultControlPoint(const BrushContext &ctx) const;
	glm::ivec3 previewControlPoint(const BrushContext &ctx) const;
	glm::ivec3 controlPoint(const BrushContext &ctx) const;
	void syncControlPoint(const BrushContext &ctx);
	void lockSegment(const BrushContext &ctx);
	void clearPendingSegments();
	static bool allowNextBezierPreview(const BrushContext &ctx, int selectedSegment, int segmentCount);
	/**
	 * @brief Place voxels along the line using raycasting
	 *
	 * Uses raycasting from reference position to cursor position, placing voxels
	 * according to the stipple pattern. Each step checks the pattern to determine
	 * if a voxel should be placed.
	 */
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	LineBrush() : Super(BrushType::Line) {
		// Initialize with solid line (all bits set)
		for (int i = 0; i < _stipplePattern.bits(); ++i) {
			_stipplePattern.set(i, true);
		}
	}
	virtual ~LineBrush() = default;
	void onActivated() override;
	bool onDeactivated() override;
	bool hasPendingChanges() const override;
	void construct() override;
	bool beginBrush(const BrushContext &ctx) override;
	void preExecute(const BrushContext &ctx, const voxel::RawVolume *volume) override;
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;
	void update(const BrushContext &ctx, double nowSeconds) override;

	/**
	 * @brief Calculate bounding box containing the line
	 * @return Region from min to max of reference and cursor positions
	 */
	voxel::Region calcRegion(const BrushContext &ctx) const override;

	/**
	 * @brief In continuous mode, update reference position for next segment
	 */
	void endBrush(BrushContext &ctx) override;
	void shutdown() override;

	void reset() override;
	bool active() const override;
	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;
	bool applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
						 uint32_t operation) override;

	/**
	 * @return True if continuous line mode is enabled
	 */
	bool continuous() const;
	bool bezier() const;
	int thickness() const;

	/**
	 * @brief Enable or disable continuous line mode
	 * @param[in] continuous If true, lines chain together
	 */
	void setContinuous(bool continuous);
	void setBezier(bool bezier);
	void setThickness(int thickness);
	void setControlPoint(const glm::ivec3 &controlPoint);
	int pendingSegmentCount() const;
	int selectedSegment() const;
	bool selectSegment(int index);
	bool removeSegment(int index);
	const BezierSegment *segment(int index) const;

	/**
	 * @brief Set a specific bit in the stipple pattern
	 * @param[in] index Bit index
	 * @param[in] value True to place voxel, false to skip
	 */
	void setStippleBit(int index, bool value);

	/**
	 * @return Reference to the stipple pattern for direct manipulation
	 */
	LineStipplePattern &stipplePattern();
};

inline bool LineBrush::continuous() const {
	return _continuous;
}

inline bool LineBrush::bezier() const {
	return _bezier;
}

inline void LineBrush::setContinuous(bool continuous) {
	_continuous = continuous;
}

inline int LineBrush::thickness() const {
	return _thickness;
}

inline void LineBrush::setThickness(int thickness) {
	_thickness = glm::clamp(thickness, 1, 64);
}

inline void LineBrush::setBezier(bool bezier) {
	if (_bezier == bezier) {
		return;
	}
	_bezier = bezier;
	_hasCustomControlPoint = false;
	markDirty();
}

inline void LineBrush::setControlPoint(const glm::ivec3 &controlPoint) {
	if (_hasCustomControlPoint && _controlPoint == controlPoint) {
		return;
	}
	_controlPoint = controlPoint;
	_hasCustomControlPoint = true;
	markDirty();
}

inline int LineBrush::pendingSegmentCount() const {
	return (int)_segments.size();
}

inline int LineBrush::selectedSegment() const {
	return _selectedSegment;
}

inline const LineBrush::BezierSegment *LineBrush::segment(int index) const {
	if (index < 0 || index >= (int)_segments.size()) {
		return nullptr;
	}
	return &_segments[index];
}

inline bool LineBrush::removeSegment(int index) {
	if (index < 0 || index >= (int)_segments.size()) {
		return false;
	}
	_segments.erase(index);
	if (_selectedSegment >= (int)_segments.size()) {
		_selectedSegment = (int)_segments.size() - 1;
	}
	markDirty();
	return true;
}

inline LineStipplePattern &LineBrush::stipplePattern() {
	return _stipplePattern;
}

inline void LineBrush::setStippleBit(int index, bool value) {
	_stipplePattern.set(index, value);
}

} // namespace voxedit
