#pragma once

#include "GLFunc.h"
#include "video/Types.h"

namespace video {

namespace _priv {

template<typename GetName, typename GetLocation>
static int fillUniforms(Id _program, ShaderUniforms& _uniforms, const std::string& _name, GLenum activeEnum, GLenum activeMaxLengthEnum, GetName getName, GetLocation getLocation, bool block) {
	int numUniforms = 0;
	glGetProgramiv(_program, activeEnum, &numUniforms);
	int uniformNameSize = 0;
	glGetProgramiv(_program, activeMaxLengthEnum, &uniformNameSize);
	char name[uniformNameSize + 1];

	for (int i = 0; i < numUniforms; i++) {
		getName(_program, i, uniformNameSize, nullptr, name);
		const int location = getLocation(_program, name);
		if (location < 0) {
			Log::warn("Could not get uniform location for %s is %i (shader %s)", name, location, _name.c_str());
			continue;
		}
		char* array = strchr(name, '[');
		if (array != nullptr) {
			*array = '\0';
		}
		_uniforms[name] = Uniform{location, block};
		Log::debug("Got uniform location for %s is %i (shader %s)", name, location, _name.c_str());
	}
	return numUniforms;
}

extern void debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam);
extern bool checkFramebufferStatus();
extern void setupLimits();
extern void setupFeatures();

}

}
