/**
 * @file
 */

#pragma once

#include "GLFunc.h"

namespace video {

class FrameBuffer {
private:
	GLuint _framebuffer;
	int _attached;
public:
	FrameBuffer();
	~FrameBuffer();

	void shutdown();

	bool isSuccessful();

	/**
	 * @note Call glViewport after the framebuffer was bound
	 */
	void bind();
	void unbind();
	/**
	 * @param[in] texture The texture handle
	 * @param[in] attachmentType Possible values are @c GL_COLOR_ATTACHMENT0..GL_COLOR_ATTACHMENTn, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT
	 */
	void attachTexture(GLuint texture, GLenum attachmentType);
	void attachRenderBuffer(GLenum internalformat, GLenum attachment, GLsizei width, GLsizei height);
	void drawBuffers(GLsizei n, const GLenum *buffers);
};

}
