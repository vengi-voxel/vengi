/**
 * @file
 */

#pragma once

#include "GLFunc.h"

namespace video {

class DepthBuffer {
public:
	DepthBuffer();
	~DepthBuffer();

	bool init(int width, int height);
	void shutdown();

	void bind();
	void unbind();
	float *read();

	inline GLuint getTexture() const {
		return _depthTexture;
	}

private:
	GLint _viewport[4] = {0, 0, 0, 0};
	GLuint _fbo = 0u;
	GLuint _depthTexture = 0u;
	int _width = 0;
	int _height = 0;
};

}
