#pragma once

#include <string>
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "GLFunc.h"
#include "core/Log.h"

namespace video {

#ifndef VERTEX_POSTFIX
#define VERTEX_POSTFIX ".vert"
#endif

#ifndef FRAGMENT_POSTFIX
#define FRAGMENT_POSTFIX ".frag"
#endif

enum ShaderType {
	SHADER_VERTEX, SHADER_FRAGMENT,

	SHADER_MAX
};

class Shader {
protected:
	GLuint _shader[SHADER_MAX];
	GLuint _program;
	bool _initialized;
	mutable bool _active;

	typedef std::unordered_map<std::string, int> ShaderVariables;
	ShaderVariables _uniforms;
	ShaderVariables _attributes;

	mutable uint32_t _time;

	std::string _name;

	int fetchUniforms();

	int fetchAttributes();

	void createProgramFromShaders();

public:
	Shader();
	virtual ~Shader();

	static int glslVersion;

	bool load(const std::string& name, const std::string& buffer, ShaderType shaderType);

	std::string getSource(ShaderType shaderType, const std::string& buffer) const;

	/**
	 * If the shaders were loaded manually via @c ::load, then you have to initialize the shader manually, too
	 */
	bool init();

	bool loadFromFile(const std::string& filename, ShaderType shaderType);

	/**
	 * @brief Loads a vertex and fragment shader for the given base filename.
	 *
	 * The filename is hand over to your @c Context implementation with the appropriate filename postfixes
	 *
	 * @see VERTEX_POSTFIX
	 * @see FRAGMENT_POSTFIX
	 */
	bool loadProgram(const std::string& filename);

	/**
	 * @brief Returns the raw shader handle
	 */
	GLuint getShader(ShaderType shaderType) const;

	/**
	 * @brief Ticks the shader
	 */
	virtual void update(uint32_t deltaTime);

	/**
	 * @brief Bind the shader program
	 *
	 * @return @c true if is is useable now, @c false if not
	 *
	 * @see
	 */
	virtual bool activate() const;

	virtual bool deactivate() const;

	int getAttributeLocation(const std::string& name) const;

	int getUniformLocation(const std::string& name) const;

	void setUniformi(const std::string& name, int value) const;
	void setUniformi(int location, int value) const;
	void setUniformi(const std::string& name, int value1, int value2) const;
	void setUniformi(int location, int value1, int value2) const;
	void setUniformi(const std::string& name, int value1, int value2, int value3) const;
	void setUniformi(int location, int value1, int value2, int value3) const;
	void setUniformi(const std::string& name, int value1, int value2, int value3, int value4) const;
	void setUniformi(int location, int value1, int value2, int value3, int value4) const;
	void setUniformf(const std::string& name, float value) const;
	void setUniformf(int location, float value) const;
	void setUniformf(const std::string& name, float value1, float value2) const;
	void setUniformf(int location, float value1, float value2) const;
	void setUniformf(const std::string& name, float value1, float value2, float value3) const;
	void setUniformf(int location, float value1, float value2, float value3) const;
	void setUniformf(const std::string& name, float value1, float value2, float value3, float value4) const;
	void setUniformf(int location, float value1, float value2, float value3, float value4) const;
	void setUniform1fv(const std::string& name, const float* values, int length) const;
	void setUniform1fv(int location, const float* values, int length) const;
	void setUniform2fv(const std::string& name, const float* values, int length) const;
	void setUniform2fv(int location, const float* values, int length) const;
	void setUniform3fv(const std::string& name, const float* values, int length) const;
	void setUniformVec3(const std::string& name, const glm::vec3& value) const;
	void setUniformVec3v(const std::string& name, const glm::vec3* value, int length) const;
	void setUniform3fv(int location, const float* values, int length) const;
	void setUniform4fv(const std::string& name, const float* values, int length) const;
	void setUniform4fv(int location, const float* values, int length) const;
	void setUniformVec4(const std::string& name, const glm::vec4& value) const;
	void setUniformVec4v(const std::string& name, const glm::vec4* value, int length) const;
	void setUniformMatrix(const std::string& name, const glm::mat4& matrix, bool transpose = false) const;
	void setUniformMatrix(int location, const glm::mat4& matrix, bool transpose = false) const;
	void setUniformMatrix(const std::string& name, const glm::mat3& matrix, bool transpose = false) const;
	void setUniformMatrix(int location, const glm::mat3& matrix, bool transpose = false) const;
	void setUniformf(const std::string& name, const glm::vec2& values) const;
	void setUniformf(int location, const glm::vec2& values) const;
	void setUniformf(const std::string& name, const glm::vec3& values) const;
	void setUniformf(int location, const glm::vec3& values) const;
	void setUniformf(const std::string& name, const glm::vec4& values) const;
	void setUniformf(int location, const glm::vec4& values) const;
	void setVertexAttribute(const std::string& name, int size, int type, bool normalize, int stride, void* buffer) const;
	void setVertexAttribute(int location, int size, int type, bool normalize, int stride, void* buffer) const;
	void setAttributef(const std::string& name, float value1, float value2, float value3, float value4) const;
	void disableVertexAttribute(const std::string& name) const;
	void disableVertexAttribute(int location) const;
	int enableVertexAttribute(const std::string& name) const;
	void enableVertexAttribute(int location) const;
	bool hasAttribute(const std::string& name) const;
	bool hasUniform(const std::string& name) const;
};

inline void Shader::setUniformi(const std::string& name, int value) const {
	const int location = getUniformLocation(name);
	setUniformi(location, value);
}

inline void Shader::setUniformi(int location, int value) const {
	glUniform1i(location, value);
	GL_checkError();
}

inline void Shader::setUniformi(const std::string& name, int value1, int value2) const {
	const int location = getUniformLocation(name);
	setUniformi(location, value1, value2);
}

inline void Shader::setUniformi(int location, int value1, int value2) const {
	glUniform2i(location, value1, value2);
	GL_checkError();
}

inline void Shader::setUniformi(const std::string& name, int value1, int value2, int value3) const {
	const int location = getUniformLocation(name);
	setUniformi(location, value1, value2, value3);
}

inline void Shader::setUniformi(int location, int value1, int value2, int value3) const {
	glUniform3i(location, value1, value2, value3);
	GL_checkError();
}

inline void Shader::setUniformi(const std::string& name, int value1, int value2, int value3, int value4) const {
	const int location = getUniformLocation(name);
	setUniformi(location, value1, value2, value3, value4);
}

inline void Shader::setUniformi(int location, int value1, int value2, int value3, int value4) const {
	glUniform4i(location, value1, value2, value3, value4);
	GL_checkError();
}

inline void Shader::setUniformf(const std::string& name, float value) const {
	const int location = getUniformLocation(name);
	setUniformf(location, value);
}

inline void Shader::setUniformf(int location, float value) const {
	glUniform1f(location, value);
	GL_checkError();
}

inline void Shader::setUniformf(const std::string& name, float value1, float value2) const {
	const int location = getUniformLocation(name);
	setUniformf(location, value1, value2);
}

inline void Shader::setUniformf(int location, float value1, float value2) const {
	glUniform2f(location, value1, value2);
	GL_checkError();
}

inline void Shader::setUniformf(const std::string& name, float value1, float value2, float value3) const {
	const int location = getUniformLocation(name);
	setUniformf(location, value1, value2, value3);
}

inline void Shader::setUniformf(int location, float value1, float value2, float value3) const {
	glUniform3f(location, value1, value2, value3);
	GL_checkError();
}

inline void Shader::setUniformf(const std::string& name, float value1, float value2, float value3, float value4) const {
	const int location = getUniformLocation(name);
	setUniformf(location, value1, value2, value3, value4);
}

inline void Shader::setUniformf(int location, float value1, float value2, float value3, float value4) const {
	glUniform4f(location, value1, value2, value3, value4);
	GL_checkError();
}

inline void Shader::setUniform1fv(const std::string& name, const float* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform1fv(location, values, length);
}

inline void Shader::setUniform1fv(int location, const float* values, int length) const {
	glUniform1fv(location, length, values);
	GL_checkError();
}

inline void Shader::setUniform2fv(const std::string& name, const float* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform2fv(location, values, length);
}

inline void Shader::setUniform2fv(int location, const float* values, int length) const {
	glUniform2fv(location, length / 2, values);
	GL_checkError();
}

inline void Shader::setUniform3fv(const std::string& name, const float* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform3fv(location, values, length);
}

inline void Shader::setUniformVec3(const std::string& name, const glm::vec3& value) const {
	const int location = getUniformLocation(name);
	glUniform3fv(location, 1, glm::value_ptr(value));
}

inline void Shader::setUniformVec3v(const std::string& name, const glm::vec3* value, int length) const {
	const int location = getUniformLocation(name);
	glUniform3fv(location, length, glm::value_ptr(*value));
}

inline void Shader::setUniform3fv(int location, const float* values, int length) const {
	glUniform3fv(location, length / 3, values);
	GL_checkError();
}

inline void Shader::setUniform4fv(const std::string& name, const float* values, int length) const {
	int location = getUniformLocation(name);
	setUniform4fv(location, values, length);
}

inline void Shader::setUniform4fv(int location, const float* values, int length) const {
	glUniform4fv(location, length / 4, values);
	GL_checkError();
}

inline void Shader::setUniformVec4(const std::string& name, const glm::vec4& value) const {
	const int location = getUniformLocation(name);
	glUniform4fv(location, 1, glm::value_ptr(value));
	GL_checkError();
}

inline void Shader::setUniformVec4v(const std::string& name, const glm::vec4* value, int length) const {
	const int location = getUniformLocation(name);
	glUniform4fv(location, length, glm::value_ptr(*value));
	GL_checkError();
}

inline void Shader::setUniformMatrix(const std::string& name, const glm::mat4& matrix, bool transpose) const {
	const int location = getUniformLocation(name);
	setUniformMatrix(location, matrix, transpose);
}

inline void Shader::setUniformMatrix(int location, const glm::mat4& matrix, bool transpose) const {
	glUniformMatrix4fv(location, 1, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrix));
	GL_checkError();
}

inline void Shader::setUniformMatrix(const std::string& name, const glm::mat3& matrix, bool transpose) const {
	const int location = getUniformLocation(name);
	setUniformMatrix(location, matrix, transpose);
}

inline void Shader::setUniformMatrix(int location, const glm::mat3& matrix, bool transpose) const {
	glUniformMatrix3fv(location, 1, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrix));
	GL_checkError();
}

inline void Shader::setUniformf(const std::string& name, const glm::vec2& values) const {
	setUniformf(name, values.x, values.y);
}

inline void Shader::setUniformf(int location, const glm::vec2& values) const {
	setUniformf(location, values.x, values.y);
}

inline void Shader::setUniformf(const std::string& name, const glm::vec3& values) const {
	setUniformf(name, values.x, values.y, values.z);
}

inline void Shader::setUniformf(int location, const glm::vec3& values) const {
	setUniformf(location, values.x, values.y, values.z);
}

inline void Shader::setUniformf(const std::string& name, const glm::vec4& values) const {
	setUniformf(name, values.x, values.y, values.z, values.w);
}

inline void Shader::setUniformf(int location, const glm::vec4& values) const {
	setUniformf(location, values.x, values.y, values.z, values.w);
}

inline void Shader::setVertexAttribute(const std::string& name, int size, int type, bool normalize, int stride, void* buffer) const {
	const int location = getAttributeLocation(name);
	if (location == -1)
		return;
	setVertexAttribute(location, size, type, normalize, stride, buffer);
}

inline void Shader::setVertexAttribute(int location, int size, int type, bool normalize, int stride, void* buffer) const {
	glVertexAttribPointer(location, size, type, normalize, stride, buffer);
	GL_checkError();
}

inline void Shader::setAttributef(const std::string& name, float value1, float value2, float value3, float value4) const {
	const int location = getAttributeLocation(name);
	glVertexAttrib4f(location, value1, value2, value3, value4);
	GL_checkError();
}

inline void Shader::disableVertexAttribute(const std::string& name) const {
	const int location = getAttributeLocation(name);
	if (location == -1)
		return;
	disableVertexAttribute(location);
}

inline void Shader::disableVertexAttribute(int location) const {
	glDisableVertexAttribArray(location);
	GL_checkError();
}

inline int Shader::enableVertexAttribute(const std::string& name) const {
	int location = getAttributeLocation(name);
	if (location == -1)
		return -1;
	enableVertexAttribute(location);
	return location;
}

inline void Shader::enableVertexAttribute(int location) const {
	glEnableVertexAttribArray(location);
	GL_checkError();
}

inline bool Shader::hasAttribute(const std::string& name) const {
	return _attributes.find(name) != _attributes.end();
}

inline bool Shader::hasUniform(const std::string& name) const {
	return _uniforms.find(name) != _uniforms.end();
}

class ShaderScope {
private:
	const Shader& _shader;
public:
	ShaderScope(const Shader& shader) :
			_shader(shader) {
		_shader.activate();
	}

	virtual ~ShaderScope() {
		_shader.deactivate();
	}
};


typedef std::shared_ptr<Shader> ShaderPtr;

}
