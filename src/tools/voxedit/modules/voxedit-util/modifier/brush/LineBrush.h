/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "LineState.h"
#include "core/collection/BitSet.h"
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

protected:
	LineState _state;					///< Cached state for detecting changes requiring preview update
	bool _continuous = false;			///< If true, end position becomes next reference position
	LineStipplePattern _stipplePattern; ///< 9-bit pattern controlling which voxels are placed

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
	void construct() override;
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

	void reset() override;

	/**
	 * @return True if continuous line mode is enabled
	 */
	bool continuous() const;

	/**
	 * @brief Enable or disable continuous line mode
	 * @param[in] continuous If true, lines chain together
	 */
	void setContinuous(bool continuous);

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

inline void LineBrush::setContinuous(bool continuous) {
	_continuous = continuous;
}

inline LineStipplePattern &LineBrush::stipplePattern() {
	return _stipplePattern;
}

inline void LineBrush::setStippleBit(int index, bool value) {
	_stipplePattern.set(index, value);
}

} // namespace voxedit
