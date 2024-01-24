/**
 * @file
 */

#pragma once

#include "AABBBrush.h"

namespace palette {
class Palette;
}

namespace voxedit {

/**
 * @brief Changes the color of existing voxels
 */
class PaintBrush : public AABBBrush {
public:
	enum class PaintMode { Replace, Brighten, Darken, Random, Variation, Max };

	static constexpr const char *PaintModeStr[] = {"Replace", "Brighten", "Darken", "Random", "Variation"};
	static_assert(lengthof(PaintModeStr) == (int)PaintBrush::PaintMode::Max, "PaintModeStr size mismatch");

private:
	using Super = AABBBrush;

	// TODO: _plane mode should ignore aabb
	// TODO: _center should work with _radius in combination with _single
	// TODO: _radius should only go into one direction (see BrushContext::_cursorFace) (only paint the surface)
	float _factor = 1.0f;
	int _variationThreshold = 3;
	bool _plane = false;
	PaintMode _paintMode = PaintMode::Replace;

protected:
	class VoxelColor {
	private:
		const voxel::Voxel _voxel;
		const palette::Palette &_palette;
		const PaintMode _paintMode;
		float _factor = 1.0f;
		int _variationThreshold = 3;

	public:
		VoxelColor(const palette::Palette &palette, const voxel::Voxel &voxel, PaintMode paintMode, float factor, int variationThreshold)
			: _voxel(voxel), _palette(palette), _paintMode(paintMode), _factor(factor), _variationThreshold(variationThreshold) {
		}
		voxel::Voxel evaluate(const voxel::Voxel &old);
	};

	bool generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &context,
				  const voxel::Region &region) override;

public:
	PaintBrush() : Super(BrushType::Paint) {
	}
	virtual ~PaintBrush() = default;

	ModifierType modifierType(ModifierType type) const override;

	PaintMode paintMode() const;
	void setPaintMode(PaintMode mode);

	void setPlane(bool plane);
	bool plane() const;

	void setVariationThreshold(int variationThreshold);
	int variationThreshold() const;

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

inline void PaintBrush::setPlane(bool plane) {
	_plane = plane;
}

inline bool PaintBrush::plane() const {
	return _plane;
}

inline ModifierType PaintBrush::modifierType(ModifierType type) const {
	return ModifierType::Paint;
}

} // namespace voxedit
