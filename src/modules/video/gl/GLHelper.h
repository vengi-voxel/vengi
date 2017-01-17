#pragma once

#include "GLFunc.h"

namespace video {

namespace _priv {

extern void debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam);
extern bool checkFramebufferStatus();
extern void setupLimits();
extern void setupFeatures();

}

}
