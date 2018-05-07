/**
 * @file
 */

#pragma once

#include "Renderer.h"

namespace video {

class ScopedBlendMode {
private:
	bool _oldEnabled;
	video::BlendMode _oldSrc;
	video::BlendMode _oldDest;
	BlendEquation _oldFunc;
public:
	ScopedBlendMode(video::BlendMode src = BlendMode::One, video::BlendMode dest = BlendMode::Zero, BlendEquation func = BlendEquation::Max);
	~ScopedBlendMode();
};

}
