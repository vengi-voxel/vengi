#pragma once

#include "Renderer.h"
#include "core/GLM.h"

namespace video {

/**
 * @brief Restore the previous scissor after leaving the scope of the object
 */
class ScopedScissor {
public:
	/**
	 * @note Keep in mind that opengl y starts from below - these are no screen coordinates, but opengl coordinates.
	 */
	ScopedScissor(int x, int y, int w, int h, int viewHeight) {
		const int y1 = viewHeight - (y + h);
		video::enable(video::State::Scissor);
		video::scissor(x, y1, w, h);
	}

	ScopedScissor(const glm::ivec2& pos, const glm::ivec2& size, int viewHeight) :
			ScopedScissor(pos.x, pos.y, size.x, size.y, viewHeight) {
	}

	ScopedScissor(const glm::ivec2& pos, int w, int h, int viewHeight) :
			ScopedScissor(pos.x, pos.y, w, h, viewHeight) {
	}

	~ScopedScissor() {
		video::disable(video::State::Scissor);
	}
};

}
