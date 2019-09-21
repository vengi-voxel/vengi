/**
 * @file
 */

#include "video/Shader.h"
#include "video/UniformBuffer.h"
#include "flextGL.h"
#include <SDL.h>

namespace video {

bool Shader::setAttributeLocation(const std::string& name, int location) {
	if (_program == InvalidId) {
		return false;
	}
	glBindAttribLocation(_program, location, name.c_str());
	checkError();
	return true;
}

bool Shader::setUniformBuffer(const std::string& name, const UniformBuffer& buffer) {
	const Uniform* uniform = getUniform(name);
	if (uniform == nullptr) {
		return false;
	}
	if (!uniform->block) {
		return false;
	}

#if 0
	const GLuint uniformBlockBinding = glGetUniformBlockIndex(_program, "name");
#else
	const GLuint uniformBlockBinding = 0;
#endif
	glUniformBlockBinding(_program, (GLuint)uniform->location, uniformBlockBinding);
	checkError();
	addUsedUniform(uniform->location);
	return buffer.bind();
}

void Shader::setUniformui(int location, unsigned int value) const {
	if (checkUniformCache(location, &value, sizeof(value))) {
		glUniform1ui(location, value);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformi(int location, int value) const {
	if (checkUniformCache(location, &value, sizeof(value))) {
		glUniform1i(location, value);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformi(int location, int value1, int value2) const {
	const int value[] = { value1, value2 };
	if (checkUniformCache(location, value, sizeof(value))) {
		glUniform2i(location, value1, value2);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformi(int location, int value1, int value2, int value3) const {
	const int value[] = { value1, value2, value3 };
	if (checkUniformCache(location, value, sizeof(value))) {
		glUniform3i(location, value1, value2, value3);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformi(int location, int value1, int value2, int value3, int value4) const {
	const int value[] = { value1, value2, value3, value4 };
	if (checkUniformCache(location, value, sizeof(value))) {
		glUniform4i(location, value1, value2, value3, value4);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform1iv(int location, const int* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(int))) {
		glUniform1iv(location, amount, values);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform2iv(int location, const int* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(int))) {
		glUniform2iv(location, amount / 2, values);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform3iv(int location, const int* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(int))) {
		glUniform3iv(location, amount / 3, values);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformIVec2v(int location, const glm::ivec2* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::ivec2))) {
		glUniform2iv(location, amount, glm::value_ptr(*value));
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformIVec3v(int location, const glm::ivec3* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::ivec3))) {
		glUniform3iv(location, amount, glm::value_ptr(*value));
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformIVec4v(int location, const glm::ivec4* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::ivec4))) {
		glUniform4iv(location, amount, glm::value_ptr(*value));
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformf(int location, float value) const {
	if (checkUniformCache(location, &value, sizeof(value))) {
		glUniform1f(location, value);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformf(int location, float value1, float value2) const {
	const float value[] = { value1, value2 };
	if (checkUniformCache(location, value, sizeof(value))) {
		glUniform2f(location, value1, value2);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformf(int location, float value1, float value2, float value3) const {
	const float value[] = { value1, value2, value3 };
	if (checkUniformCache(location, value, sizeof(value))) {
		glUniform3f(location, value1, value2, value3);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformf(int location, float value1, float value2, float value3, float value4) const {
	const float value[] = { value1, value2, value3, value4 };
	if (checkUniformCache(location, value, sizeof(value))) {
		glUniform4f(location, value1, value2, value3, value4);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform4fv(int location, const float* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(float))) {
		glUniform4fv(location, amount / 4, values);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformVec4v(int location, const glm::vec4* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::vec4))) {
		glUniform4fv(location, amount, glm::value_ptr(*value));
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformMatrixv(int location, const glm::mat4* matrixes, int amount, bool transpose) const {
	if (checkUniformCache(transpose ? -location : location, glm::value_ptr(matrixes[0]), amount * sizeof(glm::mat4))) {
		glUniformMatrix4fv(location, amount, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrixes[0]));
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformMatrixv(int location, const glm::mat3* matrixes, int amount, bool transpose) const {
	if (checkUniformCache(transpose ? -location : location, glm::value_ptr(matrixes[0]), amount * sizeof(glm::mat3))) {
		glUniformMatrix3fv(location, amount, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrixes[0]));
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform1fv(int location, const float* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(float))) {
		glUniform1fv(location, amount, values);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform2fv(int location, const float* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(float))) {
		glUniform2fv(location, amount / 2, values);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformVec2v(int location, const glm::vec2* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::vec2))) {
		glUniform2fv(location, amount, glm::value_ptr(*value));
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformVec3v(int location, const glm::vec3* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::vec3))) {
		glUniform3fv(location, amount, glm::value_ptr(*value));
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform3fv(int location, const float* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(float))) {
		glUniform3fv(location, amount / 3, values);
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setVertexAttribute(int location, int size, DataType type, bool normalize, int stride, const void* buffer) const {
	core_assert_msg(getAttributeComponents(location) == -1 || getAttributeComponents(location) == size, "%i expected, but got %i components", getAttributeComponents(location), size);
#ifdef DEBUG
#if SDL_ASSERT_LEVEL > 0
	GLint vao = -1;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
	core_assert_msg(vao > 0, "No vertex array object is bound");
#endif
#endif
	glVertexAttribPointer(location, size, std::enum_value(type), normalize, stride, buffer);
	checkError();
}

void Shader::setVertexAttributeInt(int location, int size, DataType type, int stride, const void* buffer) const {
	core_assert_msg(getAttributeComponents(location) == -1 || getAttributeComponents(location) == size, "%i expected, but got %i components", getAttributeComponents(location), size);
#ifdef DEBUG
#if SDL_ASSERT_LEVEL > 0
	GLint vao = -1;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
	core_assert_msg(vao > 0, "No vertex array object is bound");
#endif
#endif
	glVertexAttribIPointer(location, size, std::enum_value(type), stride, buffer);
	checkError();
}

void Shader::enableVertexAttributeArray(int location) const {
#ifdef DEBUG
#if SDL_ASSERT_LEVEL > 0
	GLint vao = -1;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
	core_assert_msg(vao > 0, "No vertex array object is bound");
#endif
#endif
	core_assert(location != -1);
	glEnableVertexAttribArray(location);
	checkError();
}

void Shader::disableVertexAttribute(int location) const {
#ifdef DEBUG
#if SDL_ASSERT_LEVEL > 0
	GLint vao = -1;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
	core_assert_msg(vao > 0, "No vertex array object is bound");
#endif
#endif
	core_assert(location != -1);
	glDisableVertexAttribArray(location);
	checkError();
}

bool Shader::setDivisor(int location, uint32_t divisor) const {
	if (location == -1) {
		return false;
	}
	glVertexAttribDivisor((GLuint)location, (GLuint)divisor);
	checkError();
	return true;
}

}
