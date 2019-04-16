/**
 * @file
 */

#pragma once

#include "flextGL.h"
#include "video/Types.h"

#define GL_OFFSET_CAST(i) ((void*)(i))

namespace video {

namespace _priv {

template<typename GetName, typename GetLocation>
static int fillUniforms(Id program, ShaderUniforms& uniformMap, const std::string& shaderName, GLenum activeEnum,
		GLenum activeMaxLengthEnum, GetName getName, GetLocation getLocation, bool block) {
	int numUniforms = 0;
	glGetProgramiv(program, activeEnum, &numUniforms);
	int uniformNameSize = 0;
	glGetProgramiv(program, activeMaxLengthEnum, &uniformNameSize);
	char name[4096];
	if (uniformNameSize + 1 >= sizeof(name)) {
		return 0;
	}

	const char *shaderNameC = shaderName.c_str();
	for (int i = 0; i < numUniforms; i++) {
		getName(program, i, uniformNameSize, nullptr, name);
		const int location = getLocation(program, name);
		if (location < 0) {
			Log::warn("Could not get uniform location for %s is %i (shader %s)", name, location, shaderNameC);
			continue;
		}
		char* array = strchr(name, '[');
		if (array != nullptr) {
			*array = '\0';
		}
		uniformMap.insert(std::make_pair(std::string(name), Uniform{location, block}));
		Log::debug("Got uniform location for %s is %i (shader %s)", name, location, shaderNameC);
	}
	return numUniforms;
}

#if WIN32
extern void __stdcall
#else
extern void
#endif
debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
		const GLchar* message, const GLvoid* userParam);
extern bool checkFramebufferStatus();
extern void setupLimitsAndSpecs();
extern void setupFeatures();

}

}
