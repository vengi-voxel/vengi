/**
 * @file
 */

#pragma once

#include "Types.h"
#include "video/Renderer.h"

namespace video {

class ScopedFaceCull {
private:
	video::Face _oldFace;

public:
	ScopedFaceCull(video::Face face) {
		_oldFace = video::currentCullFace();
		video::cullFace(face);
	}

	~ScopedFaceCull() {
		video::cullFace(_oldFace);
	}
};

} // namespace video
