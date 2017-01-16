#pragma once

#include "Renderer.h"
#include "core/GLM.h"

namespace video {

/**
 * @brief Restore the previous scissor after leaving the scope of the object
 */
class ScopedScissor {
private:
	GLint _scissor[4];
	GLboolean _scissorTest;
public:
	/**
	 * @note Keep in mind that opengl y starts from below - these are no screen coordinates, but opengl coordinates.
	 */
	ScopedScissor(int x, int y, int w, int h, int viewHeight) {
		const int y1 = viewHeight - (y + h);
		_scissorTest = glIsEnabled(GL_SCISSOR_TEST);
		if (!_scissorTest) {
			glGetIntegerv(GL_SCISSOR_BOX, _scissor);
			glEnable(GL_SCISSOR_TEST);
		} else {
			_scissor[0] = _scissor[1] = _scissor[2] = _scissor[3] = 0;
		}
		glScissor(x, y1, w, h);
		video::checkError();
	}

	ScopedScissor(const glm::ivec2& pos, const glm::ivec2& size, int viewHeight) :
			ScopedScissor(pos.x, pos.y, size.x, size.y, viewHeight) {
	}

	ScopedScissor(const glm::ivec2& pos, int w, int h, int viewHeight) :
			ScopedScissor(pos.x, pos.y, w, h, viewHeight) {
	}

	~ScopedScissor() {
		if (!_scissorTest) {
			glDisable(GL_SCISSOR_TEST);
		} else {
			glScissor(_scissor[0], _scissor[1], (GLsizei)_scissor[2], (GLsizei)_scissor[3]);
		}
		video::checkError();
	}
};

}
