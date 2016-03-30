#include "Shader.h"

#include "core/App.h"
#include "io/Filesystem.h"

#ifndef MAX_SHADER_VAR_NAME
#define MAX_SHADER_VAR_NAME 128
#endif

#ifdef _DEBUG
#define checkError() CheckErrorState(__FILE__, __LINE__, __PRETTY_FUNCTION__, _name.c_str());
#else
#define checkError()
#endif

namespace video {

Shader::Shader() :
		_program(0), _initialized(false), _active(false), _time(0) {
	for (int i = 0; i < SHADER_MAX; ++i) {
		_shader[i] = 0;
	}
}

Shader::~Shader() {
	for (int i = 0; i < SHADER_MAX; ++i) {
		if (_shader[i] != 0)
			glDeleteShader(_shader[i]);
	}
	if (_program != 0)
		glDeleteProgram(_program);
}

bool Shader::load(const std::string& name, const std::string& buffer, ShaderType shaderType) {
	_name = name;
	const std::string& source = getSource(shaderType, buffer);
	const GLenum glType = shaderType == SHADER_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
	checkError();

	_shader[shaderType] = glCreateShader(glType);
	const char *s = source.c_str();
	glShaderSource(_shader[shaderType], 1, (const GLchar**) &s, nullptr);
	glCompileShader(_shader[shaderType]);

	GLint status;
	glGetShaderiv(_shader[shaderType], GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE || glGetError() != GL_NO_ERROR) {
		GLint infoLogLength;
		glGetShaderiv(_shader[shaderType], GL_INFO_LOG_LENGTH, &infoLogLength);

		std::unique_ptr<GLchar> strInfoLog(new GLchar[infoLogLength + 1]);
		glGetShaderInfoLog(_shader[shaderType], infoLogLength, nullptr, strInfoLog.get());
		const std::string errorLog(strInfoLog.get(), static_cast<std::size_t>(infoLogLength));

		const char *strShaderType;
		switch (glType) {
		case GL_VERTEX_SHADER:
			strShaderType = "vertex";
			break;
		case GL_FRAGMENT_SHADER:
			strShaderType = "fragment";
			break;
		default:
			strShaderType = "unknown";
			break;
		}

		Log::error("compile failure in %s (type: %s) shader:\n%s", name.c_str(), strShaderType, errorLog.c_str());
		return false;
	}

	return true;
}

bool Shader::loadFromFile(const std::string& filename, ShaderType shaderType) {
	const std::string& buffer = core::App::getInstance()->filesystem()->load(filename);
	if (buffer.empty()) {
		Log::error("could not load shader %s", filename.c_str());
		return false;
	}

	return load(filename, buffer, shaderType);
}

/**
 * @brief Loads a vertex and fragment shader for the given base filename.
 *
 * The filename is hand over to your @c Context implementation with the appropriate filename postfixes
 *
 * @see VERTEX_POSTFIX
 * @see FRAGMENT_POSTFIX
 */
bool Shader::loadProgram(const std::string& filename) {
	const bool vertex = loadFromFile(filename + VERTEX_POSTFIX, SHADER_VERTEX);
	if (!vertex)
		return false;

	const bool fragment = loadFromFile(filename + FRAGMENT_POSTFIX, SHADER_FRAGMENT);
	if (!fragment)
		return false;

	return init();
}

bool Shader::init() {
	createProgramFromShaders();
	fetchAttributes();
	fetchUniforms();
	const bool success = _program != 0;
	_initialized = success;
	return success;
}

GLuint Shader::getShader(ShaderType shaderType) const {
	return _shader[shaderType];
}

void Shader::update(uint32_t deltaTime) {
	_time += deltaTime;
}

bool Shader::activate() const {
	glUseProgram(_program);
	checkError();
	_active = true;
	return _active;
}

bool Shader::deactivate() const {
	if (!_active) {
		return false;
	}

	glUseProgram(0);
	checkError();
	_active = false;
	_time = 0;
	return _active;
}

int Shader::getAttributeLocation(const std::string& name) const {
	ShaderVariables::const_iterator i = _attributes.find(name);
	if (i == _attributes.end()) {
		Log::error("can't find attribute %s in shader %s", name.c_str(), _name.c_str());
		return -1;
	}
	return i->second;
}

int Shader::getUniformLocation(const std::string& name) const {
	ShaderVariables::const_iterator i = _uniforms.find(name);
	if (i == _uniforms.end()) {
		Log::error("can't find uniform %s in shader %s", name.c_str(), _name.c_str());
		return -1;
	}
	return i->second;
}

int Shader::fetchUniforms() {
	char name[MAX_SHADER_VAR_NAME];
	int numUniforms = 0;
	glGetProgramiv(_program, GL_ACTIVE_UNIFORMS, &numUniforms);
	checkError();

	_uniforms.clear();
	for (int i = 0; i < numUniforms; i++) {
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveUniform(_program, i, MAX_SHADER_VAR_NAME - 1, &length, &size, &type, name);
		const int location = glGetUniformLocation(_program, name);
		_uniforms[name] = location;
		Log::debug("uniform location for %s is %i (shader %s)", name, location, _name.c_str());
	}
	return numUniforms;
}

int Shader::fetchAttributes() {
	char name[MAX_SHADER_VAR_NAME];
	int numAttributes = 0;
	glGetProgramiv(_program, GL_ACTIVE_ATTRIBUTES, &numAttributes);
	checkError();

	_attributes.clear();
	for (int i = 0; i < numAttributes; i++) {
		GLsizei length;
		GLint size;
		GLenum type;
		glGetActiveAttrib(_program, i, MAX_SHADER_VAR_NAME - 1, &length, &size, &type, name);
		const int location = glGetAttribLocation(_program, name);
		_attributes[name] = location;
		Log::debug("attribute location for %s is %i (shader %s)", name, location, _name.c_str());
	}
	return numAttributes;
}

std::string Shader::getSource(ShaderType shaderType, const std::string& buffer) const {
	std::string src;
#ifdef GL_ES_VERSION_2_0
	src.append("#version 300\n");
#else
	src.append("#version 150\n");
#endif

	std::string append(buffer);

	const std::string include = "#include";
	int index = 0;
	for (std::string::iterator i = append.begin(); i != append.end(); ++i, ++index) {
		const char *c = &append[index];
		if (*c != '#') {
			src.append(c, 1);
			continue;
		}
		if (::strncmp(include.c_str(), c, include.length())) {
			src.append(c, 1);
			continue;
		}
		for (; i != append.end(); ++i, ++index) {
			const char *cStart = &append[index];
			if (*cStart != '"')
				continue;

			++index;
			++i;
			for (; i != append.end(); ++i, ++index) {
				const char *cEnd = &append[index];
				if (*cEnd != '"')
					continue;

				const std::string includeFile(cStart + 1, cEnd);
				const std::string& includeBuffer = core::App::getInstance()->filesystem()->load(includeFile);
				if (!includeBuffer.empty()) {
					Log::error("could not load shader include %s (shader %s)", includeFile.c_str(), _name.c_str());
					break;
				}
				src.append(includeBuffer);
				break;
			}
			break;
		}
	}

	return src;
}

void Shader::createProgramFromShaders() {
	checkError();
	_program = glCreateProgram();
	checkError();

	const GLuint vert = _shader[SHADER_VERTEX];
	const GLuint frag = _shader[SHADER_FRAGMENT];

	glAttachShader(_program, vert);
	glAttachShader(_program, frag);
	checkError();

	glLinkProgram(_program);
	GLint status;
	glGetProgramiv(_program, GL_LINK_STATUS, &status);
	checkError();
	if (status == GL_TRUE)
		return;
	GLint infoLogLength;
	glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &infoLogLength);

	GLchar* strInfoLog = new GLchar[infoLogLength + 1];
	glGetProgramInfoLog(_program, infoLogLength, nullptr, strInfoLog);
	strInfoLog[infoLogLength] = '\0';
	Log::error("linker failure: %s", strInfoLog);
	glDeleteProgram(_program);
	_program = 0;
	delete[] strInfoLog;
}

}
