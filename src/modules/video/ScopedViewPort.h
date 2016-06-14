#include "GLFunc.h"

namespace video {

class ScopedViewPort {
private:
	GLint _viewport[4];
public:
	ScopedViewPort(int x, int y, int w, int h) {
		glGetIntegerv(GL_VIEWPORT, _viewport);
		glViewport(x, y, w, h);
	}

	~ScopedViewPort() {
		glViewport(_viewport[0], _viewport[1], (GLsizei)_viewport[2], (GLsizei)_viewport[3]);
	}
};

}
