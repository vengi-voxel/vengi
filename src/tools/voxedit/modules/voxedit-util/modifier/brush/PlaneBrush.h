/**
 * @file
 */

#pragma once

#include "Brush.h"

namespace voxedit {

/**
 * @brief A brush that generates voxels on a whole plane or extrudes on existing voxels
 * @ingroup Brushes
 */
class PlaneBrush : public Brush {
private:
	using Super = Brush;
	int _thickness = 1;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &context,
				  const voxel::Region &region) override;
	voxel::Region calcRegion(const BrushContext &context) const override;

public:
	PlaneBrush() : Super(BrushType::Plane) {
	}
	virtual ~PlaneBrush() = default;

	void setThickness(int thickness);
	int thickness() const;
};

inline void PlaneBrush::setThickness(int thickness) {
	_thickness = thickness;
	if (_thickness < 1)
		_thickness = 1;

	markDirty();
}

inline int PlaneBrush::thickness() const {
	return _thickness;
}

} // namespace voxedit
