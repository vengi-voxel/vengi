/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "color/ColorUtil.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxel/Region.h"
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

	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	SelectBrush()
		: Super(BrushType::Select, ModifierType::Override, ModifierType::Override | ModifierType::Erase) {
		setBrushClamping(true);
	}
	virtual ~SelectBrush() = default;

	voxel::Region calcRegion(const BrushContext &ctx) const override;

	void setSelectMode(SelectMode mode) {
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
};

} // namespace voxedit
