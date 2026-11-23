/**
 * @file
 */

#pragma once

#include "BrushPanelContext.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace command {
struct CommandExecutionListener;
}

namespace voxedit {

/**
 * @brief Settings UI for the transform brush (move, shear, scale, rotate on the active selection).
 */
class BrushPanelTransform {
public:
	void update(BrushPanelContext &ctx, command::CommandExecutionListener &listener);

private:
	math::Vec3i _moveOffset{0};
	math::Vec3i _shearOffset{0};
	math::Vec3f _scale{1.0f};
	math::Vec3f _rotation{0.0f};
	math::Vec3i _targetSize{0};
	bool _uniformScale = true;
	bool _useVoxelSize = false;
	bool _maintainAspectRatio = true;
};

} // namespace voxedit
