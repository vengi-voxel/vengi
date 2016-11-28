/**
 * @file
 */

#pragma once

#include <string>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include "GLFunc.h"
#include "core/GLM.h"
#include "core/Log.h"

namespace video {

#ifndef VERTEX_POSTFIX
#define VERTEX_POSTFIX ".vert"
#endif

#ifndef FRAGMENT_POSTFIX
#define FRAGMENT_POSTFIX ".frag"
#endif

#ifndef GEOMETRY_POSTFIX
#define GEOMETRY_POSTFIX ".geom"
#endif

enum class ShaderType : GLenum {
#ifdef GL_VERSION_4_3
	Compute = GL_COMPUTE_SHADER,
#endif
#ifdef GL_VERSION_4_0
	TesselationEval = GL_TESS_EVALUATION_SHADER,
	TesselationControl = GL_TESS_CONTROL_SHADER,
#endif
	Vertex = GL_VERTEX_SHADER,
	Fragment = GL_FRAGMENT_SHADER,
	Geometry = GL_GEOMETRY_SHADER
};

class Shader {
protected:
	typedef std::unordered_map<ShaderType, GLuint, EnumClassHash> ShaderMap;
	ShaderMap _shader;

	GLuint _program = 0u;
	bool _initialized = false;;
	mutable bool _active = false;

	typedef std::map<std::string, std::string> ShaderDefines;
	ShaderDefines _defines;

	typedef std::unordered_map<std::string, int> ShaderUniformArraySizes;
	ShaderUniformArraySizes _uniformArraySizes;

	typedef std::unordered_map<std::string, int> ShaderVariables;
	ShaderVariables _uniforms;
	ShaderVariables _attributes;

	typedef std::unordered_map<int, int> AttributeComponents;
	AttributeComponents _attributeComponents;

	mutable uint32_t _time = 0u;

	std::string _name;

	int fetchUniforms();

	int fetchAttributes();

	void createProgramFromShaders();

	std::string handleIncludes(const std::string& buffer) const;
public:
	Shader();
	virtual ~Shader();

	static int glslVersion;

	virtual void shutdown();

	bool load(const std::string& name, const std::string& buffer, ShaderType shaderType);

	std::string getSource(ShaderType shaderType, const std::string& buffer, bool finalize = true) const;

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
	bool reload();

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
	 */
	virtual bool activate() const;

	virtual bool deactivate() const;

	bool isActive() const;

	void checkAttribute(const std::string& attribute);
	void checkUniform(const std::string& uniform);
	void checkAttributes(std::initializer_list<std::string> attributes);
	void checkUniforms(std::initializer_list<std::string> uniforms);

	/**
	 * @brief Adds a new define in the form '#define value' to the shader source code
	 */
	void addDefine(const std::string& name, const std::string& value);

	void setUniformArraySize(const std::string& name, int size);
	void setAttributeComponents(int location, int size);
	int getAttributeComponents(int location) const;
	int getAttributeComponents(const std::string& name) const;

	/**
	 * @return -1 if uniform wasn't found, or no size is known. If the uniform is known, but
	 * it is no array, this might even return 0
	 */
	int getUniformArraySize(const std::string& name) const;

	int getAttributeLocation(const std::string& name) const;

	int getUniformLocation(const std::string& name) const;
	// the location of the block
	GLuint getUniformBlockLocation(const std::string& name) const;
	// how much memory is needed to store the uniform block
	GLuint getUniformBlockSize(const std::string& name) const;
	// returns a vector with offsets for the specified member names in the same order as the names
	// these offsets can be used to e.g. memcpy the data in.
	std::vector<GLint> getUniformBlockOffsets(const char **names, int amount) const;

	void setUniformui(const std::string& name, unsigned int value) const;
	void setUniformui(int location, unsigned int value) const;
	void setUniformi(const std::string& name, int value) const;
	void setUniformi(int location, int value) const;
	void setUniformi(const std::string& name, int value1, int value2) const;
	void setUniformi(int location, int value1, int value2) const;
	void setUniformi(const std::string& name, int value1, int value2, int value3) const;
	void setUniformi(int location, int value1, int value2, int value3) const;
	void setUniformi(const std::string& name, int value1, int value2, int value3, int value4) const;
	void setUniformi(int location, int value1, int value2, int value3, int value4) const;
	void setUniform1iv(const std::string& name, const int* values, int length) const;
	void setUniform1iv(int location, const int* values, int length) const;
	void setUniform2iv(const std::string& name, const int* values, int length) const;
	void setUniform2iv(int location, const int* values, int length) const;
	void setUniform3iv(int location, const int* values, int length) const;
	void setUniform3iv(const std::string& name, const int* values, int length) const;
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
	void setUniform3fv(int location, const float* values, int length) const;
	void setUniformVec2(const std::string& name, const glm::vec2& value) const;
	void setUniformVec2v(const std::string& name, const glm::vec2* value, int length) const;
	void setUniformVec3(const std::string& name, const glm::vec3& value) const;
	void setUniformVec3v(const std::string& name, const glm::vec3* value, int length) const;
	void setUniform4fv(const std::string& name, const float* values, int length) const;
	void setUniform4fv(int location, const float* values, int length) const;
	void setUniformVec4(const std::string& name, const glm::vec4& value) const;
	void setUniformVec4v(const std::string& name, const glm::vec4* value, int length) const;
	void setUniformMatrix(const std::string& name, const glm::mat4& matrix, bool transpose = false) const;
	void setUniformMatrix(int location, const glm::mat4& matrix, bool transpose = false) const;
	void setUniformMatrix(const std::string& name, const glm::mat3& matrix, bool transpose = false) const;
	void setUniformMatrix(int location, const glm::mat3& matrix, bool transpose = false) const;
	void setUniformMatrixv(const std::string& name, const glm::mat4* matrixes, int amount, bool transpose = false) const;
	void setUniformMatrixv(int location, const glm::mat4* matrixes, int amount, bool transpose = false) const;
	void setUniformMatrixv(const std::string& name, const glm::mat3* matrixes, int amount, bool transpose = false) const;
	void setUniformMatrixv(int location, const glm::mat3* matrixes, int amount, bool transpose = false) const;
	void setUniformf(const std::string& name, const glm::vec2& values) const;
	void setUniformf(int location, const glm::vec2& values) const;
	void setUniformf(const std::string& name, const glm::vec3& values) const;
	void setUniformf(int location, const glm::vec3& values) const;
	void setUniformf(const std::string& name, const glm::vec4& values) const;
	void setUniformf(int location, const glm::vec4& values) const;
	void setVertexAttribute(const std::string& name, int size, int type, bool normalize, int stride, const void* buffer) const;
	void setVertexAttribute(int location, int size, int type, bool normalize, int stride, const void* buffer) const;
	void setVertexAttributeInt(const std::string& name, int size, int type, int stride, const void* buffer) const;
	void setVertexAttributeInt(int location, int size, int type, int stride, const void* buffer) const;
	void setAttributef(const std::string& name, float value1, float value2, float value3, float value4) const;
	void disableVertexAttribute(const std::string& name) const;
	void disableVertexAttribute(int location) const;
	int enableVertexAttributeArray(const std::string& name) const;
	void enableVertexAttributeArray(int location) const;
	bool hasAttribute(const std::string& name) const;
	bool hasUniform(const std::string& name) const;
};

inline void Shader::setUniformi(const std::string& name, int value) const {
	const int location = getUniformLocation(name);
	setUniformi(location, value);
}

inline void Shader::setUniformui(const std::string& name, unsigned int value) const {
	const int location = getUniformLocation(name);
	setUniformui(location, value);
}

inline void Shader::setUniformui(int location, unsigned int value) const {
	glUniform1ui(location, value);
	GL_checkError();
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

inline void Shader::setUniform1iv(const std::string& name, const int* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform1iv(location, values, length);
}

inline void Shader::setUniform1iv(int location, const int* values, int length) const {
	glUniform1iv(location, length, values);
	GL_checkError();
}

inline void Shader::setUniform2iv(const std::string& name, const int* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform2iv(location, values, length);
}

inline void Shader::setUniform2iv(int location, const int* values, int length) const {
	glUniform2iv(location, length / 2, values);
	GL_checkError();
}

inline void Shader::setUniform3iv(int location, const int* values, int length) const {
	glUniform3iv(location, length / 3, values);
	GL_checkError();
}

inline void Shader::setUniform3iv(const std::string& name, const int* values, int length) const {
	const int location = getUniformLocation(name);
	setUniform3iv(location, values, length);
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

inline void Shader::setUniformVec2(const std::string& name, const glm::vec2& value) const {
	const int location = getUniformLocation(name);
	glUniform2fv(location, 1, glm::value_ptr(value));
}

inline void Shader::setUniformVec2v(const std::string& name, const glm::vec2* value, int length) const {
	const int location = getUniformLocation(name);
	glUniform2fv(location, length, glm::value_ptr(*value));
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
	setUniformMatrixv(name, &matrix, 1, transpose);
}

inline void Shader::setUniformMatrix(int location, const glm::mat4& matrix, bool transpose) const {
	setUniformMatrixv(location, &matrix, 1, transpose);
}

inline void Shader::setUniformMatrix(const std::string& name, const glm::mat3& matrix, bool transpose) const {
	setUniformMatrixv(name, &matrix, 1, transpose);
}

inline void Shader::setUniformMatrix(int location, const glm::mat3& matrix, bool transpose) const {
	setUniformMatrixv(location, &matrix, 1, transpose);
}

inline void Shader::setUniformMatrixv(const std::string& name, const glm::mat4* matrixes, int amount, bool transpose) const {
	const int location = getUniformLocation(name);
	setUniformMatrixv(location, matrixes, amount, transpose);
}

inline void Shader::setUniformMatrixv(int location, const glm::mat4* matrixes, int amount, bool transpose) const {
	glUniformMatrix4fv(location, amount, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrixes[0]));
	GL_checkError();
}

inline void Shader::setUniformMatrixv(const std::string& name, const glm::mat3* matrixes, int amount, bool transpose) const {
	const int location = getUniformLocation(name);
	setUniformMatrixv(location, matrixes, amount, transpose);
}

inline void Shader::setUniformMatrixv(int location, const glm::mat3* matrixes, int amount, bool transpose) const {
	glUniformMatrix3fv(location, amount, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrixes[0]));
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

inline void Shader::setVertexAttribute(const std::string& name, int size, int type, bool normalize, int stride, const void* buffer) const {
	core_assert_msg(type == GL_FLOAT || type == GL_DOUBLE, "unexpected data type given: %x", type);
	const int location = getAttributeLocation(name);
	if (location == -1) {
		return;
	}
	setVertexAttribute(location, size, type, normalize, stride, buffer);
}

inline void Shader::setVertexAttribute(int location, int size, int type, bool normalize, int stride, const void* buffer) const {
	core_assert_msg(getAttributeComponents(location) == -1 || getAttributeComponents(location) == size, "%i expected, but got %i components", getAttributeComponents(location), size);
#ifdef DEBUG
#if SDL_ASSERT_LEVEL > 0
	GLint vao = -1;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
	core_assert_msg(vao > 0, "No vertex array object is bound");
#endif
#endif
	glVertexAttribPointer(location, size, type, normalize, stride, buffer);
	GL_checkError();
}

inline void Shader::setVertexAttributeInt(int location, int size, int type, int stride, const void* buffer) const {
	core_assert_msg(getAttributeComponents(location) == -1 || getAttributeComponents(location) == size, "%i expected, but got %i components", getAttributeComponents(location), size);
#ifdef DEBUG
#if SDL_ASSERT_LEVEL > 0
	GLint vao = -1;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
	core_assert_msg(vao > 0, "No vertex array object is bound");
#endif
#endif
	glVertexAttribIPointer(location, size, type, stride, buffer);
	GL_checkError();
}

inline void Shader::setVertexAttributeInt(const std::string& name, int size, int type, int stride, const void* buffer) const {
	core_assert_msg(type != GL_FLOAT && type != GL_DOUBLE, "unexpected data type given: %x", type);
	const int location = getAttributeLocation(name);
	if (location == -1) {
		return;
	}
	setVertexAttributeInt(location, size, type, stride, buffer);
}

inline void Shader::setAttributef(const std::string& name, float value1, float value2, float value3, float value4) const {
	const int location = getAttributeLocation(name);
	if (location == -1) {
		return;
	}
	glVertexAttrib4f(location, value1, value2, value3, value4);
	GL_checkError();
}

inline void Shader::disableVertexAttribute(const std::string& name) const {
	const int location = getAttributeLocation(name);
	if (location == -1) {
		return;
	}
	disableVertexAttribute(location);
}

inline void Shader::disableVertexAttribute(int location) const {
	glDisableVertexAttribArray(location);
	GL_checkError();
}

inline int Shader::enableVertexAttributeArray(const std::string& name) const {
	int location = getAttributeLocation(name);
	if (location == -1) {
		return -1;
	}
	enableVertexAttributeArray(location);
	return location;
}

inline void Shader::enableVertexAttributeArray(int location) const {
	glEnableVertexAttribArray(location);
	GL_checkError();
}

inline int Shader::getAttributeComponents(int location) const {
	auto i = _attributeComponents.find(location);
	if (i != _attributeComponents.end()) {
		return i->second;
	}
	return -1;
}

inline int Shader::getAttributeComponents(const std::string& name) const {
	const int loc = getAttributeLocation(name);
	if (loc == -1) {
		return -1;
	}
	return getAttributeComponents(loc);
}

inline bool Shader::hasAttribute(const std::string& name) const {
	return _attributes.find(name) != _attributes.end();
}

inline bool Shader::hasUniform(const std::string& name) const {
	return _uniforms.find(name) != _uniforms.end();
}

inline bool Shader::isActive() const {
	return _active;
}

class ScopedShader {
private:
	const Shader& _shader;
public:
	ScopedShader(const Shader& shader) :
			_shader(shader) {
		_shader.activate();
	}

	virtual ~ScopedShader() {
		_shader.deactivate();
	}
};

#define shaderSetUniformIf(shader, func, var, ...) if (shader.hasUniform(var)) { shader.func(var, __VA_ARGS__); }

typedef std::shared_ptr<Shader> ShaderPtr;

}
