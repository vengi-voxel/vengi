/**
 * @file
 */

#pragma once

#include "Types.h"
#include "math/Vec2f.h"

namespace video {

/**
 * @ingroup Video
 */
class ScopedPolygonMode {
private:
	const video::PolygonMode _mode;
	const video::PolygonMode _oldMode;
	bool _offset = false;
	bool _alreadyActive = false;
public:
	ScopedPolygonMode(video::PolygonMode mode);
	ScopedPolygonMode(video::PolygonMode mode, const math::Vec2f& offset);
	~ScopedPolygonMode();
};

}
