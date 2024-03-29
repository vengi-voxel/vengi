/**
 * @file
 */

#pragma once

#include "Brush.h"

namespace voxedit {

class PlaneBrush : public Brush {
private:
	using Super = Brush;

public:
	PlaneBrush() : Super(BrushType::Plane) {
	}
	virtual ~PlaneBrush() = default;
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
				 const BrushContext &context) override;
};

} // namespace voxedit
