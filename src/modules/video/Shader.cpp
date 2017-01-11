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
#include "UniformBuffer.h"

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

Shader::Shader() {
}

Shader::~Shader() {
	core_assert_msg(_program == 0u, "Shader %s was not properly shut down", _name.c_str());
	shutdown();
}

void Shader::checkAttribute(const std::string& attribute) {
	if (!hasAttribute(attribute)) {
		Log::warn("Attribute %s missing for shader %s", attribute.c_str(), _name.c_str());
	} else {
		Log::debug("Found attribute %s for shader %s", attribute.c_str(), _name.c_str());
	}
}

void Shader::checkUniform(const std::string& uniform) {
	if (!hasUniform(uniform)) {
		Log::warn("Uniform %s missing for shader %s", uniform.c_str(), _name.c_str());
	} else {
		Log::debug("Found uniform %s for shader %s", uniform.c_str(), _name.c_str());
	}
}

void Shader::checkAttributes(std::initializer_list<std::string> attributes) {
	for (const std::string& attribute : attributes) {
		checkAttribute(attribute);
	}
}

void Shader::checkUniforms(std::initializer_list<std::string> uniforms) {
	for (const std::string& uniform : uniforms) {
		checkUniform(uniform);
	}
}

void Shader::setUniformArraySize(const std::string& name, int size) {
	_uniformArraySizes[name] = size;
}

void Shader::setAttributeComponents(int location, int size) {
	_attributeComponents[location] = size;
}

int Shader::getUniformArraySize(const std::string& name) const {
	ShaderUniformArraySizes::const_iterator i = _uniformArraySizes.find(name);
	if (i == _uniformArraySizes.end()) {
		Log::trace("can't find uniform %s in shader %s - unknown array size", name.c_str(), _name.c_str());
		return -1;
	}
	return i->second;
}

void Shader::shutdown() {
	if (_initialized) {
		core::Singleton<ShaderManager>::getInstance().unregisterShader(this);
	}

	for (auto& shader : _shader) {
		if (shader.second != 0) {
			glDeleteShader(shader.second);
		}
	}
	_shader.clear();
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
	const GLenum glType = std::enum_value(shaderType);
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

		std::unique_ptr<GLchar[]> strInfoLog(new GLchar[infoLogLength + 1]);
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
		case GL_GEOMETRY_SHADER:
			strShaderType = "geometry";
			break;
		default:
			strShaderType = "unknown";
			break;
		}

		Log::error("%s", source.c_str());
		Log::error("compile failure in %s (type: %s) shader:\n%s", name.c_str(), strShaderType, errorLog.c_str());
		glDeleteShader(_shader[shaderType]);
		return false;
	}

	return true;
}

bool Shader::loadFromFile(const std::string& filename, ShaderType shaderType) {
	const std::string& buffer = core::App::getInstance()->filesystem()->load(filename);
	if (buffer.empty()) {
		if (shaderType == ShaderType::Vertex || shaderType == ShaderType::Fragment) {
			Log::error("could not load shader %s", filename.c_str());
		}
		return false;
	}

	return load(filename, buffer, shaderType);
}

bool Shader::loadProgram(const std::string& filename) {
	const bool vertex = loadFromFile(filename + VERTEX_POSTFIX, ShaderType::Vertex);
	if (!vertex)
		return false;

	const bool fragment = loadFromFile(filename + FRAGMENT_POSTFIX, ShaderType::Fragment);
	if (!fragment)
		return false;

	// optional
	loadFromFile(filename + GEOMETRY_POSTFIX, ShaderType::Geometry);

	_name = filename;
	return init();
}

bool Shader::reload() {
	shutdown();
	return setup();
}

bool Shader::init() {
	createProgramFromShaders();
	const bool success = _program != 0u;
	_initialized = success;
	if (_initialized) {
		fetchAttributes();
		fetchUniforms();
		Log::info("Register shader: %s", _name.c_str());
		core::Singleton<ShaderManager>::getInstance().registerShader(this);
	}
	return success;
}

GLuint Shader::getShader(ShaderType shaderType) const {
	auto shader = _shader.find(shaderType);
	if (shader == _shader.end()) {
		return 0;
	}
	return shader->second;
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
#if VALIDATE_UNIFORMS > 0
	for (const auto& e : _uniforms) {
		if (_usedUniforms.find(e.second.location) == _usedUniforms.end()) {
			Log::error("Didn't set the uniform %s (shader: %s)", e.first.c_str(), _name.c_str());
		}
	}
#endif

	return _active;
}

void Shader::addDefine(const std::string& name, const std::string& value) {
	core_assert_msg(!_initialized, "Shader is already initialized");
	_defines[name] = value;
}

int Shader::getAttributeLocation(const std::string& name) const {
	ShaderAttributes::const_iterator i = _attributes.find(name);
	if (i == _attributes.end()) {
		Log::debug("can't find attribute %s in shader %s", name.c_str(), _name.c_str());
		return -1;
	}
	return i->second;
}

int Shader::getUniformLocation(const std::string& name) const {
	const Uniform* uniform = getUniform(name);
	if (uniform == nullptr) {
		return -1;
	}
	return uniform->location;
}

const Shader::Uniform* Shader::getUniform(const std::string& name) const {
	ShaderUniforms::const_iterator i = _uniforms.find(name);
	if (i == _uniforms.end()) {
		Log::debug("can't find uniform %s in shader %s", name.c_str(), _name.c_str());
		for (auto i : _uniforms) {
			Log::trace("uniform %s", i.first.c_str());
		}
		return nullptr;
	}
	return &i->second;
}

bool Shader::setUniformBuffer(const std::string& name, const UniformBuffer& buffer) {
	const Uniform* uniform = getUniform(name);
	if (uniform == nullptr) {
		return false;
	}
	if (!uniform->block) {
		return false;
	}
	glUniformBlockBinding(_program, uniform->location, 0);
	return buffer.bind();
}

GLuint Shader::getUniformBlockLocation(const std::string& name) const {
	return getUniformLocation(name);
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
	_uniforms.clear();
	int n = fillUniforms(GL_ACTIVE_UNIFORMS, GL_ACTIVE_UNIFORM_MAX_LENGTH, glGetActiveUniformName, glGetUniformLocation, false);
	n += fillUniforms(GL_ACTIVE_UNIFORM_BLOCKS, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, glGetActiveUniformBlockName, glGetUniformBlockIndex, true);
	return n;
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

std::string Shader::handleIncludes(const std::string& buffer) const {
	std::string src;
	const std::string_view include = "#include";
	int index = 0;
	for (std::string::const_iterator i = buffer.begin(); i != buffer.end(); ++i, ++index) {
		const char *c = &buffer[index];
		if (*c != '#') {
			src.append(c, 1);
			continue;
		}
		if (::strncmp(include.data(), c, include.length())) {
			src.append(c, 1);
			continue;
		}
		for (; i != buffer.end(); ++i, ++index) {
			const char *cStart = &buffer[index];
			if (*cStart != '"')
				continue;

			++index;
			++i;
			for (; i != buffer.end(); ++i, ++index) {
				const char *cEnd = &buffer[index];
				if (*cEnd != '"')
					continue;

				const std::string_view dir = core::string::extractPath(_name);
				const std::string_view includeFile(cStart + 1, (size_t)(cEnd - (cStart + 1)));
				const std::string& includeBuffer = core::App::getInstance()->filesystem()->load(core::string::concat(dir, includeFile));
				if (includeBuffer.empty()) {
					Log::error("could not load shader include %s from dir %s (shader %s)", includeFile.data(), dir.data(), _name.c_str());
				}
				src.append(includeBuffer);
				break;
			}
			break;
		}
		if (i == buffer.end()) {
			break;
		}
	}
	return src;
}

std::string Shader::getSource(ShaderType shaderType, const std::string& buffer, bool finalize) const {
	if (buffer.empty()) {
		return "";
	}
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

	// TODO: https://github.com/mattdesl/lwjgl-basics/wiki/GLSL-Versions
	std::string_view replaceIn = "in";
	std::string_view replaceOut = "out";
	std::string_view replaceTexture1D = "texture1D";
	std::string_view replaceTexture2D = "texture2D";
	std::string_view replaceTexture3D = "texture3D";
	std::string_view replaceShadow2D = "shadow2D";
	if (glslVersion < GLSLVersion::V130) {
		replaceIn = "attribute";
		replaceOut = "varying";
	} else {
		replaceTexture1D = "texture";
		replaceTexture2D = "texture";
		replaceTexture3D = "texture";
		replaceShadow2D = "texture";
	}

	src += handleIncludes(buffer);
	int level = 0;
	while (core::string::contains(src, "#include")) {
		src = handleIncludes(src);
		++level;
		if (level >= 10) {
			Log::warn("Abort shader include loop for %s", _name.c_str());
			break;
		}
	}

	if (finalize) {
		src = core::string::replaceAll(src, "$in", replaceIn);
		src = core::string::replaceAll(src, "$out", replaceOut);
		src = core::string::replaceAll(src, "$texture1D", replaceTexture1D);
		src = core::string::replaceAll(src, "$texture2D", replaceTexture2D);
		src = core::string::replaceAll(src, "$texture3D", replaceTexture3D);
		src = core::string::replaceAll(src, "$shadow2D", replaceShadow2D);
	}
	return src;
}

void Shader::createProgramFromShaders() {
	GL_checkError();
	if (_program == 0) {
		_program = glCreateProgram();
	}
	GL_checkError();

	const GLuint vert = _shader[ShaderType::Vertex];
	const GLuint frag = _shader[ShaderType::Fragment];
	const GLuint geom = _shader[ShaderType::Geometry];

	glAttachShader(_program, vert);
	glAttachShader(_program, frag);
	if (geom != 0) {
		glAttachShader(_program, geom);
	}
	GL_checkError();

	glLinkProgram(_program);
	GLint status;
	glGetProgramiv(_program, GL_LINK_STATUS, &status);
	GL_checkError();
	if (status == GL_TRUE) {
		glDetachShader(_program, vert);
		glDetachShader(_program, frag);
		if (geom != 0) {
			glDetachShader(_program, geom);
		}
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
