/**
 * @file
 */

#include "ScopedScissor.h"
#include "Renderer.h"

namespace video {

ScopedScissor::ScopedScissor(int x, int y, int w, int h) {
	_oldState = video::enable(video::State::Scissor);
	video::getScissor(_x, _y, _w, _h);
	video::scissor(x, y, w, h);
}

ScopedScissor::ScopedScissor(const glm::ivec2& pos, const glm::ivec2& size) :
		ScopedScissor(pos.x, pos.y, size.x, size.y) {
}

ScopedScissor::ScopedScissor(const glm::ivec2& pos, int w, int h) :
		ScopedScissor(pos.x, pos.y, w, h) {
}

ScopedScissor::~ScopedScissor() {
	if (!_oldState) {
		video::disable(video::State::Scissor);
	}
	video::scissor(_x, _y, _w, _h);
}

}
