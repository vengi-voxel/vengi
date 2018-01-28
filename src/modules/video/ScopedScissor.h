/**
 * @file
 */
#pragma once

#include "Renderer.h"
#include "core/GLM.h"

namespace video {

/**
 * @brief Restore the previous scissor after leaving the scope of the object
 * @ingroup Video
 */
class ScopedScissor {
private:
	int _x;
	int _y;
	int _w;
	int _h;
	bool _oldState;
public:
	ScopedScissor(int x, int y, int w, int h) {
		_oldState = video::enable(video::State::Scissor);
		video::getScissor(_x, _y, _w, _h);
		video::scissor(x, y, w, h);
	}

	ScopedScissor(const glm::ivec2& pos, const glm::ivec2& size) :
			ScopedScissor(pos.x, pos.y, size.x, size.y) {
	}

	ScopedScissor(const glm::ivec2& pos, int w, int h) :
			ScopedScissor(pos.x, pos.y, w, h) {
	}

	~ScopedScissor() {
		if (!_oldState) {
			video::disable(video::State::Scissor);
		}
		video::scissor(_x, _y, _w, _h);
	}
};

}
