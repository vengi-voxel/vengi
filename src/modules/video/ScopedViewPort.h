#pragma once

#include "Renderer.h"

namespace video {

/**
 * @brief Restore the previous viewport after leaving the scope of the object
 */
class ScopedViewPort {
private:
	GLint _viewport[4];
public:
	/**
	 * @note Keep in mind that opengl y starts from below - these are no screen coordinates, but opengl coordinates.
	 */
	ScopedViewPort(int x, int y, int w, int h) {
		glGetIntegerv(GL_VIEWPORT, _viewport);
		glViewport(x, y, w, h);
	}

	~ScopedViewPort() {
		glViewport(_viewport[0], _viewport[1], (GLsizei)_viewport[2], (GLsizei)_viewport[3]);
	}
};

}
