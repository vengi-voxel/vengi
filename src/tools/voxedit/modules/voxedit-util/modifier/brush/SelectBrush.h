/**
 * @file
 */

#pragma once

#include "voxel/Region.h"
#include <glm/vec3.hpp>

namespace voxedit {

class SceneManager;

// TODO: convert into an AABBBrush?
/**
 * @ingroup Brushes
 */
class SelectBrush {
private:
	bool _selectStartPositionValid = false;
	glm::ivec3 _selectStartPosition{0};

public:
	void start(const glm::ivec3 &startPos);
	bool active() const;
	void stop();
	voxel::Region calcSelectionRegion(const glm::ivec3 &cursorPosition) const;
};

} // namespace voxedit
