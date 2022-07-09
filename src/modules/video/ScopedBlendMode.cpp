/**
 * @file
 */

#include "ScopedBlendMode.h"
#include "Renderer.h"

namespace video {

ScopedBlendMode::ScopedBlendMode(video::BlendMode src, video::BlendMode dest, BlendEquation func) {
	video::getBlendState(_oldEnabled, _oldSrc, _oldDest, _oldFunc);
	video::enable(video::State::Blend);
	if (src != BlendMode::Max && dest != BlendMode::Max) {
		video::blendFunc(src, dest);
	}
	if (func != BlendEquation::Max) {
		video::blendEquation(func);
	}
}

ScopedBlendMode::~ScopedBlendMode() {
	if (!_oldEnabled) {
		video::disable(video::State::Blend);
	}
	video::blendFunc(_oldSrc, _oldDest);
	video::blendEquation(_oldFunc);
}

}
