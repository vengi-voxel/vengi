#include "video/Shader.h"
#include "video/UniformBuffer.h"
#include "GLFunc.h"
#include <SDL.h>

#if VALIDATE_UNIFORMS > 0
#define ADD_LOCATION(location) _usedUniforms.insert(location);
#else
#define ADD_LOCATION(location)
#endif

namespace video {

bool Shader::setUniformBuffer(const std::string& name, const UniformBuffer& buffer) {
	const Uniform* uniform = getUniform(name);
	if (uniform == nullptr) {
		return false;
	}
	if (!uniform->block) {
		return false;
	}
	glUniformBlockBinding(_program, uniform->location, 0);
	checkError();
	ADD_LOCATION(uniform->location)
	return buffer.bind();
}

void Shader::setUniformui(int location, unsigned int value) const {
	glUniform1ui(location, value);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformi(int location, int value) const {
	glUniform1i(location, value);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformi(int location, int value1, int value2) const {
	glUniform2i(location, value1, value2);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformi(int location, int value1, int value2, int value3) const {
	glUniform3i(location, value1, value2, value3);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformi(int location, int value1, int value2, int value3, int value4) const {
	glUniform4i(location, value1, value2, value3, value4);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform1iv(int location, const int* values, int length) const {
	glUniform1iv(location, length, values);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform2iv(int location, const int* values, int length) const {
	glUniform2iv(location, length / 2, values);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform3iv(int location, const int* values, int length) const {
	glUniform3iv(location, length / 3, values);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformf(int location, float value) const {
	glUniform1f(location, value);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformf(int location, float value1, float value2) const {
	glUniform2f(location, value1, value2);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformf(int location, float value1, float value2, float value3) const {
	glUniform3f(location, value1, value2, value3);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformf(int location, float value1, float value2, float value3, float value4) const {
	glUniform4f(location, value1, value2, value3, value4);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform4fv(int location, const float* values, int length) const {
	glUniform4fv(location, length / 4, values);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformVec4v(int location, const glm::vec4* value, int length) const {
	glUniform4fv(location, length, glm::value_ptr(*value));
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformMatrixv(int location, const glm::mat4* matrixes, int amount, bool transpose) const {
	glUniformMatrix4fv(location, amount, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrixes[0]));
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformMatrixv(int location, const glm::mat3* matrixes, int amount, bool transpose) const {
	glUniformMatrix3fv(location, amount, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrixes[0]));
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform1fv(int location, const float* values, int length) const {
	glUniform1fv(location, length, values);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform2fv(int location, const float* values, int length) const {
	glUniform2fv(location, length / 2, values);
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformVec2v(int location, const glm::vec2* value, int length) const {
	glUniform2fv(location, length, glm::value_ptr(*value));
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformVec3v(int location, const glm::vec3* value, int length) const {
	glUniform3fv(location, length, glm::value_ptr(*value));
	checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform3fv(int location, const float* values, int length) const {
	glUniform3fv(location, length / 3, values);
	checkError();
	ADD_LOCATION(location)
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

void Shader::setAttributef(const std::string& name, float value1, float value2, float value3, float value4) const {
	const int location = getAttributeLocation(name);
	if (location == -1) {
		return;
	}
	glVertexAttrib4f(location, value1, value2, value3, value4);
	checkError();
}

void Shader::enableVertexAttributeArray(int location) const {
	glEnableVertexAttribArray(location);
	checkError();
}

void Shader::disableVertexAttribute(int location) const {
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
