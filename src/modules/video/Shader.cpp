/**
 * @file
 */

#include "Shader.h"

#include "core/App.h"
#include "core/Common.h"
#include "core/Hash.h"
#include "core/io/Filesystem.h"
#include "Version.h"
#include "core/Var.h"
#include "core/Singleton.h"
#include "ShaderManager.h"
#include "UniformBuffer.h"
#include "video/Renderer.h"
#include "util/IncludeUtil.h"

namespace video {

#ifdef GL_ES_VERSION_3_1
// default to opengles3
int Shader::glslVersion = GLSLVersion::V310;
#else
// default to opengl4
int Shader::glslVersion = GLSLVersion::V430;
#endif

Shader::Shader() {
}

Shader::~Shader() {
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
	_uniformStateMap.clear();
	_shader.clear();
	video::deleteProgram(_program);
	_initialized = false;
	_active = false;
	markDirty();
	_time = 0;
}

void Shader::markDirty() {
	Log::debug("Mark shader %s as dirty", _name.c_str());
	_dirty = true;
}

void Shader::markClean() {
	Log::debug("Mark shader %s as clean", _name.c_str());
	_dirty = false;
}

bool Shader::load(const std::string& name, const std::string& buffer, ShaderType shaderType) {
	if (buffer.empty()) {
		return false;
	}
	_name = name;
	const std::string& source = getSource(shaderType, buffer);

	Id id = getShader(shaderType);
	if (id == InvalidId) {
		id = video::genShader(shaderType);
		if (id == InvalidId) {
			Log::error("Failed to generate shader handle for %s\n", name.c_str());
			return false;
		}
		_shader.insert(std::make_pair(shaderType, id));
	}
	if (!video::compileShader(id, shaderType, source, _name)) {
		Log::error("Failed to compile shader for %s\n", name.c_str());
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
		const bool compute = loadFromFile(filename + COMPUTE_POSTFIX, ShaderType::Compute);
		if (!compute) {
			return false;
		}
	} else {
		const bool fragment = loadFromFile(filename + FRAGMENT_POSTFIX, ShaderType::Fragment);
		if (!fragment) {
			return false;
		}

		// optional
		loadFromFile(filename + GEOMETRY_POSTFIX, ShaderType::Geometry);
	}
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

bool Shader::isActive() const {
	core_assert(!_active || video::getProgram() == _program);
	return _active;
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
	if (_recordUsedUniforms) {
		for (const auto& e : _uniforms) {
			if (_usedUniforms.find(e.second.location) == _usedUniforms.end()) {
				Log::error("Didn't set the uniform %s (shader: %s)", e.first.c_str(), _name.c_str());
			}
		}
	}

	return _active;
}

void Shader::addDefine(const std::string& name, const std::string& value) {
	core_assert_msg(!_initialized, "Shader is already initialized");
	_defines[name] = value;
}

int Shader::getAttributeLocation(const std::string& name) const {
	const int location = checkAttributeLocation(name);
	if (location == -1) {
		Log::debug("can't find attribute %s in shader %s", name.c_str(), _name.c_str());
	}
	return location;
}

int Shader::checkAttributeLocation(const std::string& name) const {
	ShaderAttributes::const_iterator i = _attributes.find(name);
	if (i == _attributes.end()) {
		return -1;
	}
	return i->second;
}

bool Shader::checkUniformCache(int location, const void* value, size_t length) const {
#if 1
	return true;
#else
	auto i = _uniformStateMap.find(location);
	const uint32_t hash = core::hash(value, length);
	if (i == _uniformStateMap.end()) {
		_uniformStateMap[location] = hash;
		return true;
	}
	const uint32_t current = i->second;
	return current != hash;
#endif
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
		for (auto uniformEntry : _uniforms) {
			Log::trace("uniform %s", uniformEntry.first.c_str());
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

/**
 * Some drivers don't support underscores in their defines...
 */
std::string Shader::validPreprocessorName(const std::string& name) {
	return core::string::replaceAll(name, "_", "");
}

std::string Shader::getSource(ShaderType shaderType, const std::string& buffer, bool finalize, std::vector<std::string>* includedFiles) const {
	if (buffer.empty()) {
		return "";
	}
	std::string src;
	src.append("#version ");
	src.append(std::to_string(glslVersion));
	src.append("\n");
	if (shaderType == ShaderType::Compute) {
		src.append("#extension GL_ARB_compute_shader : enable\n");
		src.append("#extension GL_ARB_shader_storage_buffer_object : enable\n");
		//src.append("#extension GL_ARB_compute_variable_group_size : enable\n");
	}

	core::Var::visitSorted([&] (const core::VarPtr& var) {
		if ((var->getFlags() & core::CV_SHADER) != 0) {
			src.append("#define ");
			const std::string& validName = validPreprocessorName(var->name());
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

	std::vector<std::string> includeDirs;
	includeDirs.push_back(std::string(core::string::extractPath(_name)));
	const std::pair<std::string, bool>& includesFirst = util::handleIncludes(buffer, includeDirs, includedFiles);
	src += includesFirst.first;
	int level = 0;
	while (core::string::contains(src, "#include")) {
		const std::pair<std::string, bool>& includesRecurse = util::handleIncludes(src, includeDirs, includedFiles);
		src += includesRecurse.first;
		++level;
		if (level >= 10) {
			Log::warn("Abort shader include loop for %s", _name.c_str());
			break;
		}
	}

	core::Var::visitSorted([&] (const core::VarPtr& var) {
		if ((var->getFlags() & core::CV_SHADER) != 0) {
			const std::string& validName = validPreprocessorName(var->name());
			src = core::string::replaceAll(src, var->name(), validName);
		}
	});

	if (finalize) {
		// TODO: https://github.com/mattdesl/lwjgl-basics/wiki/GLSL-Versions
		// https://www.khronos.org/opengl/wiki/GLSL_Optimizations
		// https://www.khronos.org/opengl/wiki/Type_Qualifier_(GLSL)
		std::string_view replaceIn = "in";
		std::string_view replaceOut = "out";
		std::string_view replaceWriteOnly = "writeonly";
		std::string_view replaceReadOnly = "readonly";
		std::string_view replaceRestrict = "restrict";
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

		if (glslVersion < GLSLVersion::V420) {
			// ARB_shader_image_load_store
			replaceWriteOnly = "";
			replaceReadOnly = "";
			replaceRestrict = "";
		}
		src = core::string::replaceAll(src, "$constant", "//");
		src = core::string::replaceAll(src, "$in", replaceIn);
		src = core::string::replaceAll(src, "$writeonly", replaceWriteOnly);
		src = core::string::replaceAll(src, "$readonly", replaceReadOnly);
		src = core::string::replaceAll(src, "$restrict", replaceRestrict);
		src = core::string::replaceAll(src, "$out", replaceOut);
		src = core::string::replaceAll(src, "$texture1D", replaceTexture1D);
		src = core::string::replaceAll(src, "$texture2D", replaceTexture2D);
		src = core::string::replaceAll(src, "$texture3D", replaceTexture3D);
		src = core::string::replaceAll(src, "$shadow2D", replaceShadow2D);
	}
	return src;
}

bool Shader::createProgramFromShaders() {
	if (_program == InvalidId) {
		_program = video::genProgram();
	}

	const Id comp = getShader(ShaderType::Compute);
	if (comp != InvalidId) {
		return video::linkComputeShader(_program, comp, _name);
	}

	const Id vert = getShader(ShaderType::Vertex);
	const Id frag = getShader(ShaderType::Fragment);
	const Id geom = getShader(ShaderType::Geometry);

	if (!bindTransformFeedbackVaryings(_program, _transformFormat, _transformVaryings)) {
		_transformFormat = TransformFeedbackCaptureMode::Max;
	}

	return video::linkShader(_program, vert, frag, geom, _name);
}

bool Shader::run(const glm::uvec3& workGroups, bool wait) {
	const Id comp = getShader(ShaderType::Compute);
	if (comp == InvalidId) {
		return false;
	}
	return video::runShader(_program, workGroups, wait);
}

bool Shader::transformFeedback() const {
	return _transformFormat != TransformFeedbackCaptureMode::Max;
}

void Shader::setupTransformFeedback(const std::vector<std::string>& varyings, TransformFeedbackCaptureMode format) {
	_transformVaryings = varyings;
	_transformFormat = format;
}

}
