/**
 * @file
 */

#pragma once

#include "BrushPanelContext.h"
#include <glm/vec3.hpp>

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
	glm::ivec3 _moveOffset{0};
	glm::ivec3 _shearOffset{0};
	glm::vec3 _scale{1.0f};
	glm::vec3 _rotation{0.0f};
	glm::ivec3 _targetSize{0};
	bool _dirty = false;
	bool _uniformScale = true;
	bool _useVoxelSize = false;
	bool _maintainAspectRatio = true;

	void executeTransformBrush(BrushPanelContext &ctx);
};

} // namespace voxedit
