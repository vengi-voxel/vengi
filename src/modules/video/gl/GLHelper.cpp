#include "GLHelper.h"
#include "video/Renderer.h"
#include "GLState.h"
#include "core/Common.h"
#include "core/Log.h"
#include <SDL.h>

namespace video {

namespace _priv {

static int _recompileErrors = 0;

void debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam) {
	if (id == 131218) {
		++_recompileErrors;
		if (_recompileErrors <= 10) {
			return;
		}
		_recompileErrors = 0;
	} else if (id == 131185) {
		// ignore hints that GL_STATIC_DRAW is used...
		return;
	}
	void (*log)(const char* msg, ...);
	const char* sourceStr;
	switch (source) {
	case GL_DEBUG_SOURCE_API_ARB:
		sourceStr = "api";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM_ARB:
		sourceStr = "window";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY_ARB:
		sourceStr = "third party";
		break;
	case GL_DEBUG_SOURCE_APPLICATION_ARB:
		sourceStr = "app";
		break;
	case GL_DEBUG_SOURCE_OTHER_ARB:
		sourceStr = "other";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER_ARB:
		sourceStr = "shader";
		break;
	default:
		sourceStr = "unknown";
		break;
	}
	const char* typeStr;
	switch (type) {
	case GL_DEBUG_TYPE_ERROR_ARB:
		typeStr = "ERROR";
		log = Log::error;
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
		typeStr = "DEPRECATED_BEHAVIOR";
		log = Log::warn;
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
		typeStr = "UNDEFINED_BEHAVIOR";
		log = Log::error;
		break;
	case GL_DEBUG_TYPE_PORTABILITY_ARB:
		typeStr = "PORTABILITY";
		log = Log::warn;
		break;
	case GL_DEBUG_TYPE_PERFORMANCE_ARB:
		typeStr = "PERFORMANCE";
		log = Log::warn;
		break;
	case GL_DEBUG_TYPE_OTHER_ARB:
		typeStr = "OTHER";
		log = Log::info;
		break;
	default:
		typeStr = "<unknown>";
		log = Log::debug;
		break;
	}
	const char* sevStr;
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW_ARB:
		sevStr = "LOW";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM_ARB:
		sevStr = "MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_HIGH_ARB:
		sevStr = "HIGH";
		log = Log::error;
		break;
	case GL_DEBUG_SEVERITY_NOTIFICATION_ARB:
		sevStr = "INFO";
		log = Log::debug;
		break;
	default:
		sevStr = "<unknown>";
		break;
	}
	core_assert_msg(type == GL_DEBUG_TYPE_OTHER_ARB, "GL msg type: %s, src: %s, id: %d, severity: %s\nmsg: %s", typeStr, sourceStr, id, sevStr, message);
	log("GL msg type: %s, src: %s, id: %d, severity: %s\nmsg: %s", typeStr, sourceStr, id, sevStr, message);
}

bool checkFramebufferStatus() {
	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status == GL_FRAMEBUFFER_COMPLETE) {
		return true;
	}
	switch (status) {
	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		Log::error("FB error, incomplete attachment");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		Log::error("FB error, incomplete missing attachment");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		Log::error("FB error, incomplete draw buffer");
		break;
	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		Log::error("FB error, incomplete read buffer");
		break;
	case GL_FRAMEBUFFER_UNSUPPORTED:
		Log::error("FB error, framebuffer unsupported");
		break;
	default:
		Log::error("FB error, status: %i", (int)status);
		break;
	}
	return false;
}

void setupLimits() {
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &_priv::s.limits[std::enum_value(Limit::MaxTextureSize)]);
	checkError();
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &_priv::s.limits[std::enum_value(Limit::MaxCubeMapTextureSize)]);
	checkError();
	glGetIntegerv(GL_MAX_VIEWPORT_DIMS, &_priv::s.limits[std::enum_value(Limit::MaxViewPortWidth)]);
	checkError();
	glGetIntegerv(GL_MAX_DRAW_BUFFERS, &_priv::s.limits[std::enum_value(Limit::MaxDrawBuffers)]);
	checkError();
	glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &_priv::s.limits[std::enum_value(Limit::MaxVertexAttribs)]);
	checkError();
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &_priv::s.limits[std::enum_value(Limit::MaxCombinedTextureImageUnits)]);
	checkError();
	glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &_priv::s.limits[std::enum_value(Limit::MaxVertexTextureImageUnits)]);
	checkError();
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &_priv::s.limits[std::enum_value(Limit::MaxElementIndices)]);
	checkError();
	glGetIntegerv(GL_MAX_ELEMENTS_VERTICES, &_priv::s.limits[std::enum_value(Limit::MaxElementVertices)]);
	checkError();
	if (_priv::s.glVersion.majorVersion > 3 || (_priv::s.glVersion.majorVersion == 3 && _priv::s.glVersion.minorVersion >= 2)) {
		glGetIntegerv(GL_MAX_FRAGMENT_INPUT_COMPONENTS, &_priv::s.limits[std::enum_value(Limit::MaxFragmentInputComponents)]);
		checkError();
	} else {
		_priv::s.limits[std::enum_value(Limit::MaxFragmentInputComponents)] = 60;
	}
#ifdef GL_MAX_VERTEX_UNIFORM_VECTORS
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &_priv::s.limits[std::enum_value(Limit::MaxVertexUniformComponents)]);
	checkError();
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &_priv::s.limits[std::enum_value(Limit::MaxFragmentUniformComponents)]);
	checkError();
#else
	checkError();
	glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &_priv::s.limits[std::enum_value(Limit::MaxVertexUniformComponents)]);
	checkError();
	glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_COMPONENTS, &_priv::s.limits[std::enum_value(Limit::MaxFragmentUniformComponents)]);
#endif
	checkError();
	Log::info("GL_MAX_ELEMENTS_VERTICES: %i", _priv::s.limits[std::enum_value(Limit::MaxElementVertices)]);
	Log::info("GL_MAX_ELEMENTS_INDICES: %i", _priv::s.limits[std::enum_value(Limit::MaxElementIndices)]);
}

void setupFeatures() {
	const std::vector<const char *> array[] = {
		{"_texture_compression_s3tc", "_compressed_texture_s3tc", "_texture_compression_dxt1"},
		{"_texture_compression_pvrtc", "_compressed_texture_pvrtc"},
		{},
		{"_compressed_ATC_texture", "_compressed_texture_atc"},
		{"_texture_float"},
		{"_texture_half_float"},
		{"_instanced_arrays"},
		{"_debug_output"}
	};

	int numExts;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExts);
	Log::info("OpenGL extensions:");
	for (int i = 0; i < numExts; i++) {
		const char *extensionStr = (const char *) glGetStringi(GL_EXTENSIONS, i);
		Log::info("%s", extensionStr);
	}

	for (size_t i = 0; i < SDL_arraysize(array); ++i) {
		const std::vector<const char *>& a = array[i];
		for (const char *s : a) {
			_priv::s.features[i] = SDL_GL_ExtensionSupported(s);
			if (_priv::s.features[i]) {
				break;
			}
			++s;
		}
	}

	int mask = 0;
	if (SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &mask) != -1) {
		if ((mask & SDL_GL_CONTEXT_PROFILE_CORE) != 0) {
			_priv::s.features[std::enum_value(Feature::TextureCompressionDXT)] = true;
			_priv::s.features[std::enum_value(Feature::InstancedArrays)] = true;
			_priv::s.features[std::enum_value(Feature::TextureFloat)] = true;
		}
	}

#if SDL_VIDEO_OPENGL_ES2
	_priv::s.features[std::enum_value(Feature::TextureHalfFloat)] = SDL_GL_ExtensionSupported("_texture_half_float");
#else
	_priv::s.features[std::enum_value(Feature::TextureHalfFloat)] = _priv::s.features[std::enum_value(Feature::TextureFloat)];
#endif

#if SDL_VIDEO_OPENGL_ES3
	_priv::s.features[std::enum_value(Feature::InstancedArrays)] = true;
	_priv::s.features[std::enum_value(Feature::TextureCompressionETC2)] = true;
#endif

	if (!_priv::s.features[std::enum_value(Feature::InstancedArrays)]) {
		Log::warn("instanced_arrays extension not found!");
	}
}

}

}
