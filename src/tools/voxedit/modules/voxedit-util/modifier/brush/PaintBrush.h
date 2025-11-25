/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "app/I18N.h"
#include "voxedit-util/modifier/ModifierType.h"

namespace palette {
class Palette;
}

namespace voxedit {

/**
 * @brief Changes the color of existing voxels without modifying their presence
 *
 * The PaintBrush provides various color modification modes for recoloring voxels
 * within the AABB region. Unlike other brushes, it only supports ModifierType::Paint
 * and only affects existing voxels (doesn't place new ones).
 *
 * # Paint Modes
 *
 * - **Replace**: Change all voxels to the cursor color
 * - **Brighten**: Make colors lighter by a factor
 * - **Darken**: Make colors darker by a factor
 * - **Random**: Replace with random colors from the palette
 * - **Variation**: Randomly brighten or darken for natural variation
 *
 * # Special Modes
 *
 * - **Plane Mode**: Fill all connected voxels of the same color on a face
 * - **Gradient Mode**: Create smooth color transitions across the region
 *
 * The brush automatically finds or creates new palette colors when brightening/darkening.
 *
 * @ingroup Brushes
 * @sa AABBBrush for the region spanning behavior
 */
class PaintBrush : public AABBBrush {
public:
	/**
	 * @brief Different ways to modify voxel colors
	 */
	enum class PaintMode : uint8_t { Replace, Brighten, Darken, Random, Variation, Max };

	static constexpr const char *PaintModeStr[] = {N_("Replace"), N_("Brighten"), N_("Darken"), N_("Random"),
												   N_("Variation")};
	static_assert(lengthof(PaintModeStr) == (int)PaintBrush::PaintMode::Max, "PaintModeStr size mismatch");

private:
	using Super = AABBBrush;

	float _factor = 1.0f;					   ///< Brightness factor for Brighten/Darken modes (1.0 = no change)
	int _variationThreshold = 3;			   ///< 1 in N chance to apply variation
	PaintMode _paintMode = PaintMode::Replace; ///< Active paint mode

	/**
	 * @brief Additional flags specific to PaintBrush
	 */
	enum PaintFlags : uint32_t {
		/** Fill all connected voxels with the same color as the hit voxel */
		BRUSH_MODE_PLANE = BRUSH_MODE_CUSTOM,
		/** Create smooth gradient from hit color to cursor color */
		BRUSH_MODE_GRADIENT = BRUSH_MODE_CUSTOM + 1
	};

protected:
	/**
	 * @brief Helper class that evaluates the new color for a voxel based on paint mode
	 *
	 * Encapsulates the color transformation logic for each paint mode. The evaluate()
	 * method takes an existing voxel and returns the recolored version according to
	 * the active mode and parameters.
	 */
	class VoxelColor {
	private:
		const voxel::Voxel _voxel;	 ///< Target color/voxel
		palette::Palette &_palette;	 ///< Palette for color lookups and additions
		const PaintMode _paintMode;	 ///< Active paint mode
		float _factor = 1.0f;		 ///< Brightness factor
		int _variationThreshold = 3; ///< Variation threshold

	public:
		VoxelColor(palette::Palette &palette, const voxel::Voxel &voxel, PaintMode paintMode, float factor,
				   int variationThreshold)
			: _voxel(voxel), _palette(palette), _paintMode(paintMode), _factor(factor),
			  _variationThreshold(variationThreshold) {
		}

		/**
		 * @brief Compute the new voxel color based on the paint mode
		 * @param[in] old The existing voxel to recolor
		 * @return The voxel with modified color
		 */
		voxel::Voxel evaluate(const voxel::Voxel &old);
	};

	/**
	 * @brief Apply the paint operation to all voxels in the region
	 *
	 * Iterates through the region and recolors existing voxels according to the
	 * active paint mode. Handles plane filling and gradient modes specially.
	 */
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	PaintBrush() : Super(BrushType::Paint, ModifierType::Paint, ModifierType::Paint) {
	}
	virtual ~PaintBrush() = default;

	/**
	 * @brief Override to disable AABB spanning in plane mode
	 *
	 * In plane mode, the brush immediately fills connected voxels without
	 * requiring the user to span an AABB.
	 */
	bool wantAABB() const override;

	PaintMode paintMode() const;
	void setPaintMode(PaintMode mode);

	/**
	 * @brief Enable plane fill mode - fill all connected voxels of the same color
	 */
	void setPlane();
	bool plane() const;

	/**
	 * @brief Enable gradient mode - smooth color transition across region
	 */
	void setGradient();
	bool gradient() const;

	/**
	 * @brief Set the variation threshold (1 in N chance to modify)
	 * @param[in] variationThreshold Value between 2 and 20
	 */
	void setVariationThreshold(int variationThreshold);
	int variationThreshold() const;

	/**
	 * @brief Set the brightness factor for Brighten/Darken modes
	 * @param[in] factor Value between 0.1 and 10.0 (1.0 = no change)
	 */
	void setFactor(float factor);
	float factor() const;
};

inline int PaintBrush::variationThreshold() const {
	return _variationThreshold;
}

inline float PaintBrush::factor() const {
	return _factor;
}

inline PaintBrush::PaintMode PaintBrush::paintMode() const {
	return _paintMode;
}

inline void PaintBrush::setPaintMode(PaintMode mode) {
	_paintMode = mode;
	markDirty();
}

inline bool PaintBrush::plane() const {
	return isMode(BRUSH_MODE_PLANE);
}

inline void PaintBrush::setPlane() {
	setMode(BRUSH_MODE_PLANE);
}

inline bool PaintBrush::gradient() const {
	return isMode(BRUSH_MODE_GRADIENT);
}

inline void PaintBrush::setGradient() {
	setMode(BRUSH_MODE_GRADIENT);
}

} // namespace voxedit
