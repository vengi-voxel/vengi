/**
 * @file
 */

#include "Shader.h"

#include "app/App.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Hash.h"
#include "core/GLM.h"
#include "io/Filesystem.h"
#include "Version.h"
#include "core/Assert.h"
#include "core/Var.h"
#include "core/Enum.h"
#include "core/Singleton.h"
#include "core/StringUtil.h"
#include "video/Renderer.h"
#include "util/IncludeUtil.h"
#include <glm/gtc/type_ptr.hpp>
#include "engine-config.h"

namespace video {

// GLSL version used by shader source preprocessing (#version and compatibility rewrites).
// Keep this backend-neutral so VK-only builds don't depend on GL-only GLSLVersion enums.

#ifdef USE_OPENGLES
/**
 * @brief Strip a named layout qualifier (e.g. "binding" or "set") from shader source.
 * Handles ", binding = N", "binding = N, " and standalone "binding = N" inside layout().
 */
static core::String stripLayoutQualifier(const core::String &src, const char *qualifier) {
	core::String out = src;
	// ", qualifier = N"
	{
		core::String pattern = core::String::format(", %s", qualifier);
		size_t pos = 0;
		while ((pos = out.find(pattern, pos)) != core::String::npos) {
			size_t end = pos + pattern.size();
			// skip whitespace
			while (end < out.size() && (out[end] == ' ' || out[end] == '=')) {
				++end;
			}
			while (end < out.size() && (out[end] == ' ')) {
				++end;
			}
			while (end < out.size() && out[end] >= '0' && out[end] <= '9') {
				++end;
			}
			out.erase(pos, end - pos);
		}
	}
	// "qualifier = N, "
	{
		core::String pattern = core::String::format("%s", qualifier);
		size_t pos = 0;
		while ((pos = out.find(pattern, pos)) != core::String::npos) {
			// make sure this is at a word boundary (preceded by '(' or space)
			if (pos > 0 && out[pos - 1] != '(' && out[pos - 1] != ' ') {
				pos += pattern.size();
				continue;
			}
			size_t end = pos + pattern.size();
			while (end < out.size() && out[end] == ' ') {
				++end;
			}
			if (end >= out.size() || out[end] != '=') {
				pos += pattern.size();
				continue;
			}
			++end; // skip '='
			while (end < out.size() && out[end] == ' ') {
				++end;
			}
			while (end < out.size() && out[end] >= '0' && out[end] <= '9') {
				++end;
			}
			// skip trailing ", "
			if (end < out.size() && out[end] == ',') {
				++end;
				while (end < out.size() && out[end] == ' ') {
					++end;
				}
			}
			out.erase(pos, end - pos);
		}
	}
	return out;
}
#endif

int Shader::glslVersion = 430;

Shader::Shader() {
	for (int i = 0; i < (int)ShaderType::Max; ++i) {
		_shader[i] = InvalidId;
	}
}

Shader::~Shader() {
	Shader::shutdown();
}

void Shader::shutdown() {
	for (auto& shader : _shader) {
		video::deleteShader(shader);
	}
	_uniformStateMap.clear();
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

bool Shader::load(const core::String& name, const core::String& buffer, ShaderType shaderType) {
	if (buffer.empty()) {
		return false;
	}
	_name = name;
	const core::String& source = getSource(shaderType, buffer);

	Id id = getShader(shaderType);
	if (id == InvalidId) {
		id = video::genShader(shaderType);
		if (id == InvalidId) {
			Log::error("Failed to generate shader handle for %s\n", name.c_str());
			return false;
		}
		_shader[(int)shaderType] = id;
	}
	if (!video::compileShader(id, shaderType, source, _name)) {
		_shader[(int)shaderType] = InvalidId;
		Log::error("Failed to compile shader for %s\n", name.c_str());
		return false;
	}
	return true;
}

bool Shader::loadFromFile(const core::String& filename, ShaderType shaderType) {
	const core::String& buffer = io::filesystem()->load(filename);
	if (buffer.empty()) {
		if (shaderType == ShaderType::Vertex || shaderType == ShaderType::Fragment) {
			Log::error("could not load shader %s", filename.c_str());
		}
		return false;
	}

	return load(filename, buffer, shaderType);
}

bool Shader::loadProgram(const core::String& filename) {
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
	while (_initialized) {
		shutdown();
	}
	return setup();
}

bool Shader::init() {
	createProgramFromShaders();
	const bool success = _program != InvalidId;
	_initialized = success;
	return success;
}

Id Shader::getShader(ShaderType shaderType) const {
	return _shader[(int)shaderType];
}

void Shader::update(uint32_t deltaTime) {
	_time += deltaTime;
}

bool Shader::isActive() const {
	return _active;
}

bool Shader::activate() const {
	video::useProgram(_program);
	_active = true;
	_uniformStateMap.clear();
	return _active;
}

bool Shader::deactivate() const {
	if (!_active) {
		return false;
	}

	_active = false;
	_time = 0;

	return _active;
}

void Shader::addDefine(const core::String& name, const core::String& value) {
	core_assert_msg(!_initialized, "Shader is already initialized");
	_defines.put(name, value);
}

bool Shader::checkUniformCache(int location, const void* value, int length) const {
#if 0
	return true;
#else
	auto i = _uniformStateMap.find(location);
	const uint64_t hash = core::hash(value, length);
	if (i == _uniformStateMap.end()) {
		_uniformStateMap.put(location, hash);
		return true;
	}
	const uint64_t current = i->second;
	if (current == hash) {
		return false;
	}
	_uniformStateMap.put(location, hash);
	return true;
#endif
}

core::String Shader::validPreprocessorName(const core::String& name) {
	core_assert(!name.empty());
	return core::string::replaceAll(name, "_", "");
}

core::String Shader::getSource(ShaderType shaderType, const core::String& buffer, bool finalize, core::List<core::String>* includedFiles) const {
	if (buffer.empty()) {
		return core::String::Empty;
	}
	core::String src;
	src.append("#version ");
	src.append(core::string::toString(glslVersion));
#ifdef USE_OPENGLES
	src.append(" es");
#endif

	src.append("\n");
	src.append("// ");
	src.append(_name);
	src.append("\n");
	if (shaderType == ShaderType::Compute) {
		src.append("#extension GL_ARB_compute_shader : enable\n");
		src.append("#extension GL_ARB_shader_storage_buffer_object : enable\n");
		//src.append("#extension GL_ARB_compute_variable_group_size : enable\n");
	}

#ifndef USE_OPENGLES
	if (glslVersion < 420) {
		src.append("#extension GL_ARB_shading_language_420pack : enable\n");
	}
#endif

#ifdef USE_OPENGLES
	if (shaderType == ShaderType::Vertex || shaderType == ShaderType::Fragment) {
		src.append("precision highp float;\n");
		src.append("precision highp int;\n");
		src.append("precision highp samplerCube;\n");
		src.append("precision highp sampler2D;\n");
		src.append("precision highp sampler3D;\n");
		src.append("precision highp sampler2DArray;\n");
		src.append("precision highp sampler2DArrayShadow;\n");
	}
#endif

	for (auto i = _defines.begin(); i != _defines.end(); ++i) {
		src.append("#ifndef ");
		src.append(i->key);
		src.append("\n");
		src.append("#define ");
		src.append(i->key);
		src.append(" ");
		src.append(i->value);
		src.append("\n");
		src.append("#endif\n");
	}

	core::List<core::String> includeDirs;
	includeDirs.insert(core::String(core::string::extractDir(_name)));
	const core::Pair<core::String, bool>& includesFirst = util::handleIncludes(_name, buffer, includeDirs, includedFiles);
	src += includesFirst.first;
	int level = 0;
	while (core::string::contains(src, "#include")) {
		const core::Pair<core::String, bool>& includesRecurse = util::handleIncludes(_name, src, includeDirs, includedFiles);
		src += includesRecurse.first;
		++level;
		if (level >= 10) {
			Log::warn("Abort shader include loop for %s", _name.c_str());
			break;
		}
	}

	if (finalize) {
		// TODO: RENDERER: https://github.com/mattdesl/lwjgl-basics/wiki/GLSL-Versions
		// https://www.khronos.org/opengl/wiki/GLSL_Optimizations
		// https://www.khronos.org/opengl/wiki/Type_Qualifier_(GLSL)
		core::String replaceIn = "in";
		core::String replaceOut = "out";
		core::String replaceWriteOnly = "writeonly";
		core::String replaceReadOnly = "readonly";
		core::String replaceRestrict = "restrict";
		core::String replaceTexture1D = "texture1D";
		core::String replaceTexture2D = "texture2D";
		core::String replaceTexture3D = "texture3D";
		core::String replaceShadow2D = "shadow2D";
#ifndef USE_OPENGLES
		if (glslVersion < 130) {
			replaceIn = "attribute";
			replaceOut = "varying";
		} else
#endif
		{
			replaceTexture1D = "texture";
			replaceTexture2D = "texture";
			replaceTexture3D = "texture";
			replaceShadow2D = "texture";
		}

		if (glslVersion < 420) {
			// ARB_shader_image_load_store
			replaceWriteOnly = "";
			replaceReadOnly = "";
			replaceRestrict = "";
		}
		src = core::string::replaceAll(src, "$constant", "#define");
		src = core::string::replaceAll(src, "$in", replaceIn);
		src = core::string::replaceAll(src, "$writeonly", replaceWriteOnly);
		src = core::string::replaceAll(src, "$readonly", replaceReadOnly);
		src = core::string::replaceAll(src, "$restrict", replaceRestrict);
		src = core::string::replaceAll(src, "$out", replaceOut);
		src = core::string::replaceAll(src, "$texture1D", replaceTexture1D);
		src = core::string::replaceAll(src, "$texture2D", replaceTexture2D);
		src = core::string::replaceAll(src, "$texture3D", replaceTexture3D);
		src = core::string::replaceAll(src, "$shadow2D", replaceShadow2D);

#ifdef USE_OPENGLES
		if (glslVersion < 310) {
			src = stripLayoutQualifier(src, "binding");
			src = stripLayoutQualifier(src, "set");
		}
#endif
	}
	return src;
}

bool Shader::createProgramFromShaders() {
	if (_program == InvalidId) {
		_program = video::genProgram();
		setObjectName(_program, video::ObjectNameType::Program, _name);
	}

	const Id comp = getShader(ShaderType::Compute);
	if (comp != InvalidId) {
		return video::linkComputeShader(_program, comp, _name);
	}

	const Id vert = getShader(ShaderType::Vertex);
	setObjectName(vert, video::ObjectNameType::Shader, _name);
	const Id frag = getShader(ShaderType::Fragment);
	setObjectName(frag, video::ObjectNameType::Shader, _name);
	const Id geom = getShader(ShaderType::Geometry);
	setObjectName(geom, video::ObjectNameType::Shader, _name);

	return video::linkShader(_program, vert, frag, geom, _name);
}

bool Shader::run(const glm::uvec3& workGroups, MemoryBarrierType wait) {
	const Id comp = getShader(ShaderType::Compute);
	if (comp == InvalidId) {
		return false;
	}
	return video::runShader(_program, workGroups, wait);
}

ScopedShader::ScopedShader(const Shader& shader) :
		_shader(shader), _oldShader(getProgram()) {
	_alreadyActive = _shader.isActive();
	_shader.activate();
}

ScopedShader::~ScopedShader() {
	if (_alreadyActive) {
		return;
	}
	_shader.deactivate();
	useProgram(_oldShader);
}

void Shader::setUniformi(int location, int value) const {
	if (checkUniformCache(location, &value, sizeof(value))) {
		video::setUniformi(location, value);
	}
}

}
