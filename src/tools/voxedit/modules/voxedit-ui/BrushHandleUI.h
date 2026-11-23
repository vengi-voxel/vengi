#include "math/Vec2f.h"
/**
 * @file
 */
#pragma once
namespace video {
class Camera;
}
namespace voxedit {
class Modifier;
bool drawBrushHandles(Modifier &modifier, const video::Camera &camera, float headerSize, const math::Vec2f &dpiScale,
					  int viewportId);
} // namespace voxedit
