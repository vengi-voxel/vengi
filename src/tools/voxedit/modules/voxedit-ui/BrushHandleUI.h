/**
 * @file
 */
#pragma once
#include <glm/vec2.hpp>
namespace video {
class Camera;
}
namespace voxedit {
class Modifier;
bool drawBrushHandles(Modifier &modifier, const video::Camera &camera, float headerSize, const glm::vec2 &dpiScale,
					  int viewportId);
} // namespace voxedit
