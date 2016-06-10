/**
 * @file
 */

#include "Shader.h"

#include "core/App.h"
#include "core/Common.h"
#include "io/Filesystem.h"
#include "GLVersion.h"
#include "core/Var.h"
#include "core/Singleton.h"
#include "ShaderManager.h"

#ifndef MAX_SHADER_VAR_NAME
#define MAX_SHADER_VAR_NAME 128
#endif

namespace video {

#ifdef GL_ES_VERSION_2_0
// default to opengles3
int Shader::glslVersion = GLSLVersion::V300;
#else
// default to opengl3
int Shader::glslVersion = GLSLVersion::V330;
#endif

Shader::Shader() :
		_program(0), _initialized(false), _active(false), _time(0) {
	for (int i = 0; i < SHADER_MAX; ++i) {
		_shader[i] = 0;
	}
	core::Singleton<ShaderManager>::getInstance().registerShader(this);
}

Shader::~Shader() {
	core::Singleton<ShaderManager>::getInstance().unregisterShader(this);
	shutdown();
}

void Shader::shutdown() {
	for (int i = 0; i < SHADER_MAX; ++i) {
		if (_shader[i] != 0) {
			glDeleteShader(_shader[i]);
			_shader[i] = 0;
		}
	}
	if (_program != 0) {
		glDeleteProgram(_program);
		_program = 0;
	}
	_initialized = false;
	_active = false;
	_time = 0;
}

bool Shader::load(const std::string& name, const std::string& buffer, ShaderType shaderType) {
	_name = name;
	const std::string& source = getSource(shaderType, buffer);
	const GLenum glType = shaderType == SHADER_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
	GL_checkError();

	if (_shader[shaderType] == 0) {
		_shader[shaderType] = glCreateShader(glType);
	}
	const char *s = source.c_str();
	glShaderSource(_shader[shaderType], 1, (const GLchar**) &s, nullptr);
	glCompileShader(_shader[shaderType]);

	GLint status;
	glGetShaderiv(_shader[shaderType], GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint infoLogLength = 0;
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
		glDeleteShader(_shader[shaderType]);
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

bool Shader::loadProgram(const std::string& filename) {
	const bool vertex = loadFromFile(filename + VERTEX_POSTFIX, SHADER_VERTEX);
	if (!vertex)
		return false;

	const bool fragment = loadFromFile(filename + FRAGMENT_POSTFIX, SHADER_FRAGMENT);
	if (!fragment)
		return false;

	_name = filename;
	return init();
}

bool Shader::reload() {
	const std::string name = _name;
	return loadProgram(name);
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
	GL_checkError();
	_active = true;
	return _active;
}

bool Shader::deactivate() const {
	if (!_active) {
		return false;
	}

	GL_checkError();
	_active = false;
	_time = 0;
	return _active;
}

void Shader::addDefine(const std::string& name, const std::string& value) {
	_defines[name] = value;
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

GLuint Shader::getUniformBlockLocation(const std::string& name) const {
	return glGetUniformBlockIndex(_program, name.c_str());
}

GLuint Shader::getUniformBlockSize(const std::string& name) const {
	GLint blockSize;
	GLuint loc = getUniformBlockLocation(name);
	glGetActiveUniformBlockiv(_program, loc, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
	return blockSize;
}

std::vector<GLint> Shader::getUniformBlockOffsets(const char **names, int amount) const {
	std::vector<GLint> offsets(amount);
	GLuint indices[amount];
	glGetUniformIndices(_program, amount, names, indices);
	glGetActiveUniformsiv(_program, amount, indices, GL_UNIFORM_OFFSET, &offsets[0]);
	return offsets;
}

int Shader::fetchUniforms() {
	char name[MAX_SHADER_VAR_NAME];
	int numUniforms = 0;
	glGetProgramiv(_program, GL_ACTIVE_UNIFORMS, &numUniforms);
	GL_checkError();

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
	GL_checkError();

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
	src.append("#version ");
	src.append(std::to_string(glslVersion));
	src.append("\n");
	if (glslVersion < GLSLVersion::V140) {
		//src.append("#extension GL_EXT_draw_instanced : enable\n");
	}

	core::Var::visitSorted([&] (const core::VarPtr& var) {
		if ((var->getFlags() & core::CV_SHADER) != 0) {
			src.append("#define ");
			src.append(var->name());
			src.append(" ");
			std::string val;
			if (var->typeIsBool()) {
				val = var->boolVal() ? "1" : "0";
			} else {
				val = var->strVal();
			}
			src.append(val);
			src.append("\n");
		}
	});

	for (auto i = _defines.begin(); i != _defines.end(); ++i) {
		src.append("#ifndef ");
		src.append(i->first);
		src.append("\n");
		src.append("#define ");
		src.append(i->first);
		src.append(" ");
		src.append(i->second);
		src.append("\n");
		src.append("#endif\n");
	}

	std::string append(buffer);

	// TODO: https://github.com/mattdesl/lwjgl-basics/wiki/GLSL-Versions
	std::string replaceIn = "in";
	std::string replaceOut = "out";
	std::string replaceTexture2D = "texture2D";
	if (glslVersion < GLSLVersion::V130) {
		replaceIn = "attribute";
		replaceOut = "varying";
	}

	if (glslVersion >= GLSLVersion::V150) {
		replaceTexture2D = "texture";
	}

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

				const std::string& dir = core::string::extractPath(_name);
				const std::string includeFile(cStart + 1, cEnd);
				const std::string& includeBuffer = core::App::getInstance()->filesystem()->load(dir + includeFile);
				if (includeBuffer.empty()) {
					Log::error("could not load shader include %s from dir %s (shader %s)", includeFile.c_str(), dir.c_str(), _name.c_str());
				}
				src.append(includeBuffer);
				break;
			}
			break;
		}
		if (i == append.end()) {
			break;
		}
	}

	src = core::string::replaceAll(src, "$in", replaceIn);
	src = core::string::replaceAll(src, "$out", replaceOut);
	src = core::string::replaceAll(src, "$texture2D", replaceTexture2D);

	return src;
}

void Shader::createProgramFromShaders() {
	GL_checkError();
	if (_program == 0) {
		_program = glCreateProgram();
	}
	GL_checkError();

	const GLuint vert = _shader[SHADER_VERTEX];
	const GLuint frag = _shader[SHADER_FRAGMENT];

	glAttachShader(_program, vert);
	glAttachShader(_program, frag);
	GL_checkError();

	glLinkProgram(_program);
	GLint status;
	glGetProgramiv(_program, GL_LINK_STATUS, &status);
	GL_checkError();
	if (status == GL_TRUE) {
		glDetachShader(_program, vert);
		glDetachShader(_program, frag);
		return;
	}
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
