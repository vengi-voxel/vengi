/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "app/I18NMarkers.h"
#include "ui/IconsLucide.h"
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

	static constexpr const char *PaintModeIcons[] = {ICON_LC_REPLACE, ICON_LC_SUN, ICON_LC_MOON, ICON_LC_SHUFFLE,
													 ICON_LC_BLEND};
	static_assert(lengthof(PaintModeIcons) == (int)PaintBrush::PaintMode::Max, "PaintModeIcons size mismatch");

private:
	using Super = AABBBrush;

	float _strength = 1.0f;					   ///< Brightness factor for Brighten/Darken modes (1.0 = no change)
	int _variationChance = 3;			   ///< 1 in N chance to apply variation
	PaintMode _paintMode = PaintMode::Replace; ///< Active paint mode

	/**
	 * @brief Additional flags specific to PaintBrush
	 */
	enum PaintFlags : uint32_t {
		/** Fill all connected voxels with the same color as the hit voxel */
		BRUSH_MODE_FLOOD_FILL = BRUSH_MODE_CUSTOM,
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
		float _strength = 1.0f;		 ///< Brightness factor
		int _variationChance = 3; ///< Variation threshold

	public:
		VoxelColor(palette::Palette &palette, const voxel::Voxel &voxel, PaintMode paintMode, float strength,
				   int variationChance)
			: _voxel(voxel), _palette(palette), _paintMode(paintMode), _strength(strength),
			  _variationChance(variationChance) {
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
	 * @brief Override to disable box spanning in flood fill mode
	 */
	bool wantBox() const override;

	PaintMode paintMode() const;
	void setPaintMode(PaintMode mode);

	/**
	 * @brief Enable flood fill mode - fill connected voxels of the same color on a face
	 */
	void setFloodFill();
	bool floodFill() const;

	/**
	 * @brief Enable gradient mode - smooth color transition across region
	 */
	void setGradient();
	bool gradient() const;

	/**
	 * @brief Set the variation threshold (1 in N chance to modify)
	 * @param[in] variationChance Value between 2 and 20
	 */
	void setVariationChance(int variationChance);
	int variationChance() const;

	/**
	 * @brief Set the brightness factor for Brighten/Darken modes
	 * @param[in] strength Value between 0.1 and 10.0 (1.0 = no change)
	 */
	void setStrength(float strength);
	float strength() const;
};

inline int PaintBrush::variationChance() const {
	return _variationChance;
}

inline float PaintBrush::strength() const {
	return _strength;
}

inline PaintBrush::PaintMode PaintBrush::paintMode() const {
	return _paintMode;
}

inline void PaintBrush::setPaintMode(PaintMode mode) {
	_paintMode = mode;
	markDirty();
}

inline bool PaintBrush::floodFill() const {
	return isMode(BRUSH_MODE_FLOOD_FILL);
}

inline void PaintBrush::setFloodFill() {
	setMode(BRUSH_MODE_FLOOD_FILL);
}

inline bool PaintBrush::gradient() const {
	return isMode(BRUSH_MODE_GRADIENT);
}

inline void PaintBrush::setGradient() {
	setMode(BRUSH_MODE_GRADIENT);
}

} // namespace voxedit
