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
	enum class PaintMode { Replace, Brighten, Darken, Random, Max };

	static constexpr const char *PaintModeStr[] = {"Replace", "Brighten", "Darken", "Random"};
	static_assert(lengthof(PaintModeStr) == (int)PaintBrush::PaintMode::Max, "PaintModeStr size mismatch");

private:
	using Super = AABBBrush;

	float _factor = 1.0f;
	bool _plane = false;
	PaintMode _paintMode = PaintMode::Replace;

protected:
	class VoxelColor {
	private:
		const voxel::Voxel _voxel;
		const palette::Palette &_palette;
		const PaintMode _paintMode;
		float _factor = 1.0f;

	public:
		VoxelColor(const palette::Palette &palette, const voxel::Voxel &voxel, PaintMode paintMode, float factor)
			: _voxel(voxel), _palette(palette), _paintMode(paintMode), _factor(factor) {
		}
		voxel::Voxel evaluate(const voxel::Voxel &old) const;
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

	void setFactor(float factor);
	float factor() const;
};

inline void PaintBrush::setFactor(float factor) {
	_factor = factor;
}

inline float PaintBrush::factor() const {
	return _factor;
}

inline PaintBrush::PaintMode PaintBrush::paintMode() const {
	return _paintMode;
}

inline void PaintBrush::setPaintMode(PaintMode mode) {
	_paintMode = mode;
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
