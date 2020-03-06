/**
 * @file
 */

#pragma once

#include "Types.h"
#include <glm/fwd.hpp>

namespace video {

/**
 * @ingroup Video
 */
class ScopedPolygonMode {
private:
	const video::PolygonMode _mode;
	const video::PolygonMode _oldMode;
	bool _offset = false;
public:
	ScopedPolygonMode(video::PolygonMode mode);
	ScopedPolygonMode(video::PolygonMode mode, const glm::vec2& offset);
	~ScopedPolygonMode();
};

}
