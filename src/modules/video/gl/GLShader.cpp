/**
 * @file
 */

#include "core/Log.h"
#include "video/RendererInterface.h"
#include "video/Shader.h"
#include "video/UniformBuffer.h"
#include "core/Assert.h"
#include "core/GLM.h"
#include "flextGL.h"
#include "core/sdl/SDLSystem.h"
#include <glm/gtc/type_ptr.hpp>

namespace video {

int32_t Shader::getUniformBufferOffset(const char *name) {
	GLuint index;
	const GLchar *uniformNames[1];
	uniformNames[0] = name;
	core_assert(glGetUniformIndices != nullptr);
	glGetUniformIndices(_program, 1, uniformNames, &index);
	checkError();
	if (index == GL_INVALID_INDEX) {
		Log::error("Could not query uniform index for %s", name);
		return -1;
	}
	GLint offset;
	core_assert(glGetActiveUniformsiv != nullptr);
	glGetActiveUniformsiv(_program, 1, &index, GL_UNIFORM_OFFSET, &offset);
	checkError();
	GLint type;
	glGetActiveUniformsiv(_program, 1, &index, GL_UNIFORM_TYPE, &type);
	checkError();
	GLint size;
	glGetActiveUniformsiv(_program, 1, &index, GL_UNIFORM_SIZE, &size); // array length, not actual type size;
	checkError();
	GLint matrixStride;
	glGetActiveUniformsiv(_program, 1, &index, GL_UNIFORM_MATRIX_STRIDE, &matrixStride);
	checkError();
	GLint arrayStride;
	glGetActiveUniformsiv(_program, 1, &index, GL_UNIFORM_ARRAY_STRIDE, &arrayStride);
	checkError();
	Log::debug("%s: offset: %i, type: %i, size: %i, matrixStride: %i, arrayStride: %i", name, offset, type, size, matrixStride, arrayStride);
	return offset;
}

bool Shader::setUniformBuffer(const core::String& name, const UniformBuffer& buffer) {
	const Uniform* uniform = getUniform(name);
	if (uniform == nullptr) {
		Log::error("%s is no uniform", name.c_str());
		return false;
	}
	if (!uniform->block) {
		Log::error("%s is no uniform buffer", name.c_str());
		return false;
	}

	if (uniform->size != (int)buffer.size()) {
		Log::error("Uniform buffer %s: size %i differs from uploaded structure size %i", name.c_str(), uniform->size, (int)buffer.size());
		return false;
	}

	core_assert(glUniformBlockBinding != nullptr);
	glUniformBlockBinding(_program, (GLuint)uniform->blockIndex, (GLuint)uniform->blockBinding);
	checkError();
	addUsedUniform(uniform->location);
	return buffer.bind(uniform->blockIndex);
}

void Shader::setUniformi(int location, int value) const {
	if (checkUniformCache(location, &value, sizeof(value))) {
		core_assert(glUniform1i != nullptr);
		glUniform1i(location, value);
		checkError();
	}
	addUsedUniform(location);
}

}
