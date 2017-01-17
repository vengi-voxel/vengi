/**
 * @file
 */

#include "Shader.h"

#include "core/App.h"
#include "core/Common.h"
#include "io/Filesystem.h"
#include "Version.h"
#include "core/Var.h"
#include "core/Singleton.h"
#include "ShaderManager.h"
#include "UniformBuffer.h"
#include "video/gl/GLFunc.h"

#ifndef MAX_SHADER_VAR_NAME
#define MAX_SHADER_VAR_NAME 128
#endif

#if VALIDATE_UNIFORMS > 0
#define ADD_LOCATION(location) _usedUniforms.insert(location);
#else
#define ADD_LOCATION(location)
#endif

namespace video {

template<typename GetName, typename GetLocation>
static int fillUniforms(Id _program, Shader::ShaderUniforms& _uniforms, const std::string& _name, GLenum activeEnum, GLenum activeMaxLengthEnum, GetName getName, GetLocation getLocation, bool block) {
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
		_uniforms[name] = Shader::Uniform{location, block};
		Log::debug("Got uniform location for %s is %i (shader %s)", name, location, _name.c_str());
	}
	return numUniforms;
}

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
	core_assert_msg(_program == InvalidId, "Shader %s was not properly shut down", _name.c_str());
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
		video::deleteShader(shader.second);
	}
	_shader.clear();
	video::deleteProgram(_program);
	_initialized = false;
	_active = false;
	_time = 0;
}

bool Shader::load(const std::string& name, const std::string& buffer, ShaderType shaderType) {
	_name = name;
	const std::string& source = getSource(shaderType, buffer);
	video::checkError();

	if (_shader[shaderType] == InvalidId) {
		_shader[shaderType] = video::genShader(shaderType);
	}
	const char *s = source.c_str();
	glShaderSource(_shader[shaderType], 1, (const GLchar**) &s, nullptr);
	glCompileShader(_shader[shaderType]);

	GLint status;
	glGetShaderiv(_shader[shaderType], GL_COMPILE_STATUS, &status);
	if (!status) {
		GLint infoLogLength = 0;
		glGetShaderiv(_shader[shaderType], GL_INFO_LOG_LENGTH, &infoLogLength);

		std::unique_ptr<GLchar[]> strInfoLog(new GLchar[infoLogLength + 1]);
		glGetShaderInfoLog(_shader[shaderType], infoLogLength, nullptr, strInfoLog.get());
		const std::string errorLog(strInfoLog.get(), static_cast<std::size_t>(infoLogLength));

		const char *strShaderType;
		switch (shaderType) {
		case ShaderType::Vertex:
			strShaderType = "vertex";
			break;
		case ShaderType::Fragment:
			strShaderType = "fragment";
			break;
		case ShaderType::Geometry:
			strShaderType = "geometry";
			break;
		default:
			strShaderType = "unknown";
			break;
		}

		Log::error("%s", source.c_str());
		Log::error("compile failure in %s (type: %s) shader:\n%s", name.c_str(), strShaderType, errorLog.c_str());
		video::deleteShader(_shader[shaderType]);
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
	if (!vertex) {
		return false;
	}

	const bool fragment = loadFromFile(filename + FRAGMENT_POSTFIX, ShaderType::Fragment);
	if (!fragment) {
		return false;
	}

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
	const bool success = _program != InvalidId;
	_initialized = success;
	if (_initialized) {
		fetchAttributes();
		fetchUniforms();
		Log::info("Register shader: %s", _name.c_str());
		core::Singleton<ShaderManager>::getInstance().registerShader(this);
	}
	return success;
}

Id Shader::getShader(ShaderType shaderType) const {
	auto shader = _shader.find(shaderType);
	if (shader == _shader.end()) {
		return InvalidId;
	}
	return shader->second;
}

void Shader::update(uint32_t deltaTime) {
	_time += deltaTime;
}

bool Shader::activate() const {
	video::useProgram(_program);
	_active = true;
	return _active;
}

bool Shader::deactivate() const {
	if (!_active) {
		return false;
	}

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

uint32_t Shader::getUniformBlockLocation(const std::string& name) const {
	return getUniformLocation(name);
}

uint32_t Shader::getUniformBlockSize(const std::string& name) const {
	GLint blockSize;
	uint32_t loc = getUniformBlockLocation(name);
	glGetActiveUniformBlockiv(_program, loc, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
	return blockSize;
}

std::vector<int> Shader::getUniformBlockOffsets(const char **names, int amount) const {
	std::vector<int> offsets(amount);
	uint32_t indices[amount];
	glGetUniformIndices(_program, amount, names, indices);
	glGetActiveUniformsiv(_program, amount, indices, GL_UNIFORM_OFFSET, &offsets[0]);
	return offsets;
}

int Shader::fetchUniforms() {
	_uniforms.clear();
	int n = fillUniforms(_program, _uniforms, _name, GL_ACTIVE_UNIFORMS, GL_ACTIVE_UNIFORM_MAX_LENGTH, glGetActiveUniformName, glGetUniformLocation, false);
	n += fillUniforms(_program, _uniforms, _name, GL_ACTIVE_UNIFORM_BLOCKS, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, glGetActiveUniformBlockName, glGetUniformBlockIndex, true);
	return n;
}

int Shader::fetchAttributes() {
	char name[MAX_SHADER_VAR_NAME];
	int numAttributes = 0;
	glGetProgramiv(_program, GL_ACTIVE_ATTRIBUTES, &numAttributes);
	video::checkError();

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
	if (_program == InvalidId) {
		_program = video::genProgram();
	}

	const Id vert = _shader[ShaderType::Vertex];
	const Id frag = _shader[ShaderType::Fragment];
	const Id geom = _shader[ShaderType::Geometry];

	glAttachShader(_program, vert);
	glAttachShader(_program, frag);
	if (geom != InvalidId) {
		glAttachShader(_program, geom);
	}

	glLinkProgram(_program);
	GLint status;
	glGetProgramiv(_program, GL_LINK_STATUS, &status);
	video::checkError();
	if (status) {
		glDetachShader(_program, vert);
		glDetachShader(_program, frag);
		if (geom != InvalidId) {
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
	video::deleteProgram(_program);
	delete[] strInfoLog;
}

void Shader::setUniformui(int location, unsigned int value) const {
	glUniform1ui(location, value);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformi(int location, int value) const {
	glUniform1i(location, value);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformi(int location, int value1, int value2) const {
	glUniform2i(location, value1, value2);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformi(int location, int value1, int value2, int value3) const {
	glUniform3i(location, value1, value2, value3);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformi(int location, int value1, int value2, int value3, int value4) const {
	glUniform4i(location, value1, value2, value3, value4);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform1iv(int location, const int* values, int length) const {
	glUniform1iv(location, length, values);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform2iv(int location, const int* values, int length) const {
	glUniform2iv(location, length / 2, values);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform3iv(int location, const int* values, int length) const {
	glUniform3iv(location, length / 3, values);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformf(int location, float value) const {
	glUniform1f(location, value);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformf(int location, float value1, float value2) const {
	glUniform2f(location, value1, value2);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformf(int location, float value1, float value2, float value3) const {
	glUniform3f(location, value1, value2, value3);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformf(int location, float value1, float value2, float value3, float value4) const {
	glUniform4f(location, value1, value2, value3, value4);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform4fv(int location, const float* values, int length) const {
	glUniform4fv(location, length / 4, values);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformVec4v(int location, const glm::vec4* value, int length) const {
	glUniform4fv(location, length, glm::value_ptr(*value));
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformMatrixv(int location, const glm::mat4* matrixes, int amount, bool transpose) const {
	glUniformMatrix4fv(location, amount, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrixes[0]));
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformMatrixv(int location, const glm::mat3* matrixes, int amount, bool transpose) const {
	glUniformMatrix3fv(location, amount, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(matrixes[0]));
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform1fv(int location, const float* values, int length) const {
	glUniform1fv(location, length, values);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform2fv(int location, const float* values, int length) const {
	glUniform2fv(location, length / 2, values);
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformVec2v(int location, const glm::vec2* value, int length) const {
	glUniform2fv(location, length, glm::value_ptr(*value));
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniformVec3v(int location, const glm::vec3* value, int length) const {
	glUniform3fv(location, length, glm::value_ptr(*value));
	video::checkError();
	ADD_LOCATION(location)
}

void Shader::setUniform3fv(int location, const float* values, int length) const {
	glUniform3fv(location, length / 3, values);
	video::checkError();
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
	video::checkError();
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
	video::checkError();
}

void Shader::setAttributef(const std::string& name, float value1, float value2, float value3, float value4) const {
	const int location = getAttributeLocation(name);
	if (location == -1) {
		return;
	}
	glVertexAttrib4f(location, value1, value2, value3, value4);
	video::checkError();
}

void Shader::enableVertexAttributeArray(int location) const {
	glEnableVertexAttribArray(location);
	video::checkError();
}

void Shader::disableVertexAttribute(int location) const {
	glDisableVertexAttribArray(location);
	video::checkError();
}

bool Shader::setDivisor(int location, uint32_t divisor) const {
	if (location == -1) {
		return false;
	}
	glVertexAttribDivisor((GLuint)location, (GLuint)divisor);
	video::checkError();
	return true;
}

}
