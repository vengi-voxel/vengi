/**
 * @file
 */

#pragma once

#include "Brush.h"

namespace voxedit {

class LineBrush : public Brush {
private:
	using Super = Brush;

public:
	virtual ~LineBrush() = default;
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx) override;
};

} // namespace voxedit
