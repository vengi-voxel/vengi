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
#include "video/Renderer.h"

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

	Id& id = _shader[shaderType];
	if (id == InvalidId) {
		id = video::genShader(shaderType);
	}
	if (!video::compileShader(id, shaderType, source, _name)) {
		Log::error("compile failure in %s\n", name.c_str());
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
	video::checkError();
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
	video::checkError();
	return init();
}

bool Shader::reload() {
	shutdown();
	return setup();
}

bool Shader::init() {
	video::checkError();
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

const Uniform* Shader::getUniform(const std::string& name) const {
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

int Shader::fetchUniforms() {
	_uniforms.clear();
	return video::fetchUniforms(_program, _uniforms, _name);
}

int Shader::fetchAttributes() {
	_attributes.clear();
	return video::fetchAttributes(_program, _attributes, _name);
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

/**
 * Some drivers don't support underscores in their defines...
 */
std::string Shader::validGLSLPreprocessorName(const std::string& name) {
	return core::string::replaceAll(name, "_", "");
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
			const std::string& validName = validGLSLPreprocessorName(var->name());
			src.append(validName);
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

	core::Var::visitSorted([&] (const core::VarPtr& var) {
		if ((var->getFlags() & core::CV_SHADER) != 0) {
			const std::string& validName = validGLSLPreprocessorName(var->name());
			src = core::string::replaceAll(src, var->name(), validName);
		}
	});

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

	video::linkShader(_program, vert, frag, geom);
}

}
