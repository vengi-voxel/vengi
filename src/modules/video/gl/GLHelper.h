/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "core/collection/DynamicArray.h"
#include "flextGL.h"
#include "video/ShaderTypes.h"
#include "video/gl/GLVersion.h"

namespace video {

namespace _priv {

int fillUniforms(Id program, ShaderUniforms& uniformMap, const core::String& shaderName, bool block);

#if defined(_WIN32) || defined(__CYGWIN__)
extern void __stdcall
#else
extern void
#endif
debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
		const GLchar* message, const GLvoid* userParam);
extern GLenum checkFramebufferStatus(video::Id fbo);
extern void setupLimitsAndSpecs();
extern void setupFeatures(GLVersion version);

}

}
