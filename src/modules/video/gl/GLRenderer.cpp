#include "video/Renderer.h"
#include "GLTypes.h"
#include "GLFunc.h"
#include "core/Common.h"
#include "core/Log.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>

namespace video {

ProfilerGPU::ProfilerGPU(const std::string& name, uint16_t maxSamples) :
		_name(name), _maxSampleCount(maxSamples) {
	core_assert(maxSamples > 0);
	_samples.reserve(_maxSampleCount);
}

ProfilerGPU::~ProfilerGPU() {
	core_assert_msg(_id == 0u, "Forgot to shutdown gpu profiler: %s", _name.c_str());
	shutdown();
}

const std::vector<double>& ProfilerGPU::samples() const {
	return _samples;
}

bool ProfilerGPU::init() {
	glGenQueries(1, &_id);
	return _id != InvalidId;
}

void ProfilerGPU::shutdown() {
	if (_id != InvalidId) {
		glDeleteQueries(1, &_id);
		_id = InvalidId;
	}
}

void ProfilerGPU::enter() {
	if (_id == InvalidId) {
		return;
	}
	core_assert(_state == 0 || _state == 2);

	if (_state == 0) {
		glBeginQuery(GL_TIME_ELAPSED, _id);
		_state = 1;
	}
}

void ProfilerGPU::leave() {
	if (_id == InvalidId) {
		return;
	}
	core_assert(_state == 1 || _state == 2);

	if (_state == 1) {
		glEndQuery(GL_TIME_ELAPSED);
		_state = 2;
	} else if (_state == 2) {
		GLint availableResults = 0;
		glGetQueryObjectiv(_id, GL_QUERY_RESULT_AVAILABLE, &availableResults);
		if (availableResults > 0) {
			_state = 0;
			GLuint64 time = 0;
			glGetQueryObjectui64v(_id, GL_QUERY_RESULT, &time);
			const double timed = double(time);
			_samples[_sampleCount & (_maxSampleCount - 1)] = timed;
			++_sampleCount;
			_max = std::max(_max, timed);
			_min = std::min(_min, timed);
			_avg = _avg * 0.5 + timed / 1e9 * 0.5;
		}
	}
}

class GLDebug {
private:
	static int _recompileErrors;
	static bool _enabled;
#if defined(GL_ARB_debug_output)
	static void debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam) {
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
#endif

public:
	static void enable(DebugSeverity s) {
#if defined(GL_ARB_debug_output)
		GLenum glSeverity = GL_DONT_CARE;
		switch (s) {
		case DebugSeverity::High:
			glSeverity = GL_DEBUG_SEVERITY_HIGH_ARB;
			break;
		case DebugSeverity::Medium:
			glSeverity = GL_DEBUG_SEVERITY_MEDIUM_ARB;
			break;
		default:
		case DebugSeverity::Low:
			glSeverity = GL_DEBUG_SEVERITY_LOW_ARB;
			break;
		}
		if (glDebugMessageControlARB != nullptr) {
			glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, glSeverity, 0, nullptr, GL_TRUE);
			if (!_enabled) {
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
				glDebugMessageCallbackARB(debugOutputCallback, nullptr);
				checkError();
				_enabled = true;
				Log::info("enable opengl debug messages");
			}
		}
#else
		Log::warn("Opengl debug extensions are not implemented");
#endif
	}

	static void disable() {
#if defined(GL_ARB_debug_output)
		glDisable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
		checkError();
		_enabled = false;
		Log::info("disable opengl debug messages");
#endif
	}

	static bool isEnabled() {
		return _enabled;
	}
};

bool GLDebug::_enabled = false;
int GLDebug::_recompileErrors = 0;

#ifdef DEBUG
static const char* translateError(GLenum glError) {
#define GL_ERROR_TRANSLATE(e) case e: return #e;
	switch (glError) {
	/* openGL errors */
	GL_ERROR_TRANSLATE(GL_INVALID_ENUM)
	GL_ERROR_TRANSLATE(GL_INVALID_VALUE)
	GL_ERROR_TRANSLATE(GL_INVALID_OPERATION)
	GL_ERROR_TRANSLATE(GL_OUT_OF_MEMORY)
	default:
		return "UNKNOWN";
	}
#undef GL_ERROR_TRANSLATE
}
#endif

void checkError() {
#ifdef DEBUG
	/* check gl errors (can return multiple errors) */
	for (;;) {
		const GLenum glError = glGetError();
		if (glError == GL_NO_ERROR) {
			break;
		}

		core_assert_msg(glError == GL_NO_ERROR, "GL err: %s => %i", translateError(glError), glError);
	}
#endif
}

namespace _priv {
	struct GLState {
		glm::vec4 clearColor;
		bool depthMask = false;
		Face cullFace = Face::Max;
		CompareFunc depthFunc = CompareFunc::Max;
		Id programHandle = InvalidId;
		Id vertexArrayHandle = InvalidId;
		glm::vec2 polygonOffset;
		Face polygonModeFace = Face::Max;
		PolygonMode polygonMode = PolygonMode::Max;
		BlendMode blendSrc = BlendMode::Max;
		BlendMode blendDest = BlendMode::Max;
		TextureUnit textureUnit = TextureUnit::Max;
		Id textureHandle = InvalidId;
		int viewportX = 0;
		int viewportY = 0;
		int viewportW = 0;
		int viewportH = 0;
		int scissorX = 0;
		int scissorY = 0;
		int scissorW = 0;
		int scissorH = 0;
		bool states[std::enum_value(State::Max)] = {};
		Id bufferHandle[std::enum_value(VertexBufferType::Max)] = {};
		Id bufferBaseHandle[std::enum_value(VertexBufferType::Max)] = {};
		Id framebufferHandle = InvalidId;
		Id framebufferTextureHandle = InvalidId;
		glm::vec2 smoothedLineWidth = glm::vec2(-1.0f);
		glm::vec2 aliasedLineWidth = glm::vec2(-1.0f);
		bool lineAntialiasing = false;
		float lineWidth = 0.0f;
	};
	static GLState s;

	static const struct Formats {
		uint8_t bits;
		GLenum internalFormat;
		GLenum dataFormat;
		GLenum dataType;
	} textureFormats[] = {
		{32, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE},
		{24, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE},
		{32, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8}
	};
	static_assert(std::enum_value(TextureFormat::Max) == (int)SDL_arraysize(textureFormats), "Array sizes don't match Max");

	static GLenum ShaderTypes[] {
		GL_VERTEX_SHADER,
		GL_FRAGMENT_SHADER,
		GL_GEOMETRY_SHADER
	};
	static_assert(std::enum_value(ShaderType::Max) == (int)SDL_arraysize(ShaderTypes), "Array sizes don't match Max");

	static GLenum FrameBufferModes[] {
		GL_READ_FRAMEBUFFER,
		GL_DRAW_FRAMEBUFFER,
		GL_FRAMEBUFFER
	};
	static_assert(std::enum_value(FrameBufferMode::Max) == (int)SDL_arraysize(FrameBufferModes), "Array sizes don't match Max");

	static GLenum VertexBufferModes[] {
		GL_STATIC_DRAW,
		GL_DYNAMIC_DRAW,
		GL_STREAM_DRAW
	};
	static_assert(std::enum_value(VertexBufferMode::Max) == (int)SDL_arraysize(VertexBufferModes), "Array sizes don't match Max");

	static GLenum VertexBufferTypes[] {
		GL_ARRAY_BUFFER,
		GL_ELEMENT_ARRAY_BUFFER,
		GL_UNIFORM_BUFFER
	};
	static_assert(std::enum_value(VertexBufferType::Max) == (int)SDL_arraysize(VertexBufferTypes), "Array sizes don't match Max");

	static GLenum States[] {
		0,
		GL_DEPTH_TEST,
		GL_CULL_FACE,
		GL_BLEND,
		GL_POLYGON_OFFSET_FILL,
		GL_POLYGON_OFFSET_POINT,
		GL_POLYGON_OFFSET_LINE,
		GL_SCISSOR_TEST,
		GL_MULTISAMPLE,
		GL_LINE_SMOOTH
	};
	static_assert(std::enum_value(State::Max) == (int)SDL_arraysize(States), "Array sizes don't match Max");

	static GLenum TextureTypes[] {
		GL_TEXTURE_2D,
		GL_TEXTURE_2D_ARRAY,
		GL_TEXTURE_CUBE_MAP
	};
	static_assert(std::enum_value(TextureType::Max) == (int)SDL_arraysize(TextureTypes), "Array sizes don't match Max");

	static GLenum TextureWraps[] {
		GL_CLAMP_TO_EDGE,
		GL_REPEAT
	};
	static_assert(std::enum_value(TextureWrap::Max) == (int)SDL_arraysize(TextureWraps), "Array sizes don't match Max");

	static GLenum BlendModes[] {
		GL_ZERO,
		GL_ONE,
		GL_SRC_COLOR,
		GL_ONE_MINUS_SRC_COLOR,
		GL_SRC_ALPHA,
		GL_ONE_MINUS_SRC_ALPHA,
		GL_DST_ALPHA,
		GL_ONE_MINUS_DST_ALPHA,
		GL_DST_COLOR,
		GL_ONE_MINUS_DST_COLOR
	};
	static_assert(std::enum_value(BlendMode::Max) == (int)SDL_arraysize(BlendModes), "Array sizes don't match Max");

	static GLenum CompareFuncs[] {
		GL_NEVER,
		GL_LESS,
		GL_EQUAL,
		GL_LEQUAL,
		GL_GREATER,
		GL_NOTEQUAL,
		GL_GEQUAL,
		GL_ALWAYS
	};
	static_assert(std::enum_value(CompareFunc::Max) == (int)SDL_arraysize(CompareFuncs), "Array sizes don't match Max");

	static GLenum PolygonModes[] {
		GL_POINT,
		GL_LINE,
		GL_FILL
	};
	static_assert(std::enum_value(PolygonMode::Max) == (int)SDL_arraysize(PolygonModes), "Array sizes don't match Max");

	static GLenum Faces[] {
		GL_FRONT,
		GL_BACK,
		GL_FRONT_AND_BACK
	};
	static_assert(std::enum_value(Face::Max) == (int)SDL_arraysize(Faces), "Array sizes don't match Max");

	static GLenum Primitives[] {
		GL_POINTS,
		GL_LINES,
		GL_TRIANGLES
	};
	static_assert(std::enum_value(Primitive::Max) == (int)SDL_arraysize(Primitives), "Array sizes don't match Max");

	static GLenum TextureUnits[] {
		GL_TEXTURE0,
		GL_TEXTURE1,
		GL_TEXTURE2,
		GL_TEXTURE3,
		GL_TEXTURE4,
		GL_TEXTURE5,
		GL_TEXTURE6
	};
	static_assert(std::enum_value(TextureUnit::Max) == (int)SDL_arraysize(TextureUnits), "Array sizes don't match Max");

	static GLenum DataTypes[] {
		GL_DOUBLE,
		GL_FLOAT,
		GL_UNSIGNED_BYTE,
		GL_BYTE,
		GL_UNSIGNED_SHORT,
		GL_SHORT,
		GL_UNSIGNED_INT,
		GL_INT
	};
	static_assert(std::enum_value(DataType::Max) == (int)SDL_arraysize(DataTypes), "Array sizes don't match Max");
}

static inline bool checkFramebufferStatus() {
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

bool bindDepthTexture(int textureIndex, DepthBufferMode mode, Id depthTexture) {
	const bool depthCompare = mode == DepthBufferMode::DEPTH_CMP;
	const bool depthAttachment = mode == DepthBufferMode::DEPTH || depthCompare;
	if (depthAttachment) {
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0, textureIndex);
		clear(video::ClearFlag::Depth);
	} else {
		glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, depthTexture, 0, textureIndex);
		clear(video::ClearFlag::Color | video::ClearFlag::Depth);
	}

	if (!checkFramebufferStatus()) {
		return false;
	}
	checkError();
	return true;
}

void readBuffer(GBufferTextureType textureType) {
	glReadBuffer(GL_COLOR_ATTACHMENT0 + textureType);
	checkError();
}

bool setupDepthbuffer(Id fbo, DepthBufferMode mode) {
	const Id prev = video::bindFramebuffer(video::FrameBufferMode::Default, fbo);
	const bool depthCompare = mode == DepthBufferMode::DEPTH_CMP;
	const bool depthAttachment = mode == DepthBufferMode::DEPTH || depthCompare;

	if (depthAttachment) {
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	} else {
		const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);
	}
	checkError();
	bindFramebuffer(video::FrameBufferMode::Default, prev);
	return true;
}

bool setupGBuffer(Id fbo, const glm::ivec2& dimension, Id* textures, size_t texCount, Id depthTexture) {
	const Id prev = video::bindFramebuffer(video::FrameBufferMode::Default, fbo);

	for (std::size_t i = 0; i < texCount; ++i) {
		video::bindTexture(video::TextureUnit::Upload, video::TextureType::Texture2D, textures[i]);
		// we are going to write vec3 into the out vars in the shaders
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, dimension.x, dimension.y, 0, GL_RGB, GL_FLOAT, nullptr);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, textures[i], 0);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	bindTexture(video::TextureUnit::Upload, video::TextureType::Texture2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, dimension.x, dimension.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(SDL_arraysize(drawBuffers), drawBuffers);

	const bool retVal = checkFramebufferStatus();
	bindFramebuffer(video::FrameBufferMode::Default, prev);
	return retVal;
}

bool setupCubemap(Id handle, const image::ImagePtr images[6]) {
	bindTexture(video::TextureUnit::Upload, video::TextureType::TextureCube, handle);

	static const GLenum types[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};

	for (unsigned int i = 1; i <= 6; i++) {
		const image::ImagePtr& img = images[i];
		const GLenum mode = img->depth() == 4 ? GL_RGBA : GL_RGB;
		glTexImage2D(types[i - 1], 0, mode, img->width(), img->height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img->data());
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return true;
}

float lineWidth(float width) {
	if (_priv::s.smoothedLineWidth.x < 0.0f) {
		GLdouble buf[2];
		glGetDoublev(GL_SMOOTH_LINE_WIDTH_RANGE, buf);
		_priv::s.smoothedLineWidth.x = (float)buf[0];
		_priv::s.smoothedLineWidth.y = (float)buf[1];
		glGetDoublev(GL_ALIASED_LINE_WIDTH_RANGE, buf);
		_priv::s.aliasedLineWidth.x = (float)buf[0];
		_priv::s.aliasedLineWidth.y = (float)buf[1];
		// TODO GL_SMOOTH_LINE_WIDTH_GRANULARITY
	}
	if (glm::abs(_priv::s.lineWidth - width) < glm::epsilon<float>()) {
		return _priv::s.lineWidth;
	}
	const float oldWidth = _priv::s.lineWidth;
	if (_priv::s.lineAntialiasing) {
		glLineWidth((GLfloat)glm::clamp(width, _priv::s.smoothedLineWidth.x, _priv::s.smoothedLineWidth.y));
	} else {
		glLineWidth((GLfloat)glm::clamp(width, _priv::s.aliasedLineWidth.x, _priv::s.aliasedLineWidth.y));
	}
	checkError();
	_priv::s.lineWidth = width;
	return oldWidth;
}

bool clearColor(const glm::vec4& clearColor) {
	if (_priv::s.clearColor == clearColor) {
		return false;
	}
	_priv::s.clearColor = clearColor;
	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	checkError();
	return true;
}

void clear(ClearFlag flag) {
	GLbitfield glValue = 0;
	if ((flag & ClearFlag::Color) == ClearFlag::Color) {
		glValue |= GL_COLOR_BUFFER_BIT;
	}
	if ((flag & ClearFlag::Stencil) == ClearFlag::Stencil) {
		glValue |= GL_STENCIL_BUFFER_BIT;
	}
	if ((flag & ClearFlag::Depth) == ClearFlag::Depth) {
		glValue |= GL_DEPTH_BUFFER_BIT;
	}
	if (glValue == 0) {
		return;
	}
	glClear(glValue);
	checkError();
}

bool viewport(int x, int y, int w, int h) {
	if (_priv::s.viewportX == x && _priv::s.viewportY == y && _priv::s.viewportW == w && _priv::s.viewportH == h) {
		return false;
	}
	_priv::s.viewportX = x;
	_priv::s.viewportY = y;
	_priv::s.viewportW = w;
	_priv::s.viewportH = h;
	glViewport((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h);
	checkError();
	return true;
}

void getViewport(int& x, int& y, int& w, int& h) {
	x = _priv::s.viewportX;
	y = _priv::s.viewportY;
	w = _priv::s.viewportW;
	h = _priv::s.viewportH;
}

bool scissor(int x, int y, int w, int h) {
	if (_priv::s.scissorX == x && _priv::s.scissorY == y && _priv::s.scissorW == w && _priv::s.scissorH == h) {
		return false;
	}
	_priv::s.scissorX = x;
	_priv::s.scissorY = y;
	_priv::s.scissorW = w;
	_priv::s.scissorH = h;
	glScissor((GLint)x, (GLint)y, (GLsizei)w, (GLsizei)h);
	checkError();
	return true;
}

bool enable(State state) {
	if (state == State::DepthMask) {
		if (_priv::s.depthMask) {
			return true;
		}
		glDepthMask(GL_TRUE);
		checkError();
		_priv::s.depthMask = true;
		return false;
	}

	const int stateIndex = std::enum_value(state);
	if (_priv::s.states[stateIndex]) {
		return true;
	}
	_priv::s.states[stateIndex] = true;
	glEnable(_priv::States[stateIndex]);
	checkError();
	return false;
}

bool disable(State state) {
	if (state == State::DepthMask) {
		if (!_priv::s.depthMask) {
			return false;
		}
		glDepthMask(GL_FALSE);
		checkError();
		_priv::s.depthMask = false;
		return true;
	}

	const int stateIndex = std::enum_value(state);
	if (!_priv::s.states[stateIndex]) {
		return false;
	}
	_priv::s.states[stateIndex] = false;
	glDisable(_priv::States[stateIndex]);
	checkError();
	return true;
}

bool cullFace(Face face) {
	if (_priv::s.cullFace == face) {
		return false;
	}
	const GLenum glFace = _priv::Faces[std::enum_value(face)];
	glCullFace(glFace);
	checkError();
	_priv::s.cullFace = face;
	return true;
}

bool depthFunc(CompareFunc func) {
	if (_priv::s.depthFunc == func) {
		return false;
	}
	glDepthFunc(_priv::CompareFuncs[std::enum_value(func)]);
	checkError();
	_priv::s.depthFunc = func;
	return true;
}

bool blendFunc(BlendMode src, BlendMode dest) {
	if (_priv::s.blendSrc == src && _priv::s.blendDest == dest) {
		return false;
	}
	_priv::s.blendSrc = src;
	_priv::s.blendDest = dest;
	const GLenum glSrc = _priv::BlendModes[std::enum_value(src)];
	const GLenum glDest = _priv::BlendModes[std::enum_value(dest)];
	glBlendFunc(glSrc, glDest);
	checkError();
	return true;
}

bool polygonMode(Face face, PolygonMode mode) {
	if (_priv::s.polygonModeFace == face && _priv::s.polygonMode == mode) {
		return false;
	}
	_priv::s.polygonModeFace = face;
	_priv::s.polygonMode = mode;
	const GLenum glMode = _priv::PolygonModes[std::enum_value(mode)];
	const GLenum glFace = _priv::Faces[std::enum_value(face)];
	glPolygonMode(glFace, glMode);
	checkError();
	return true;
}

bool polygonOffset(const glm::vec2& offset) {
	if (_priv::s.polygonOffset == offset) {
		return false;
	}
	glPolygonOffset(offset.x, offset.y);
	checkError();
	_priv::s.polygonOffset = offset;
	return true;
}

bool bindTexture(TextureUnit unit, TextureType type, Id handle) {
	bool changeUnit = false;
	if (_priv::s.textureUnit != unit) {
		const GLenum glUnit = _priv::TextureUnits[std::enum_value(unit)];
		glActiveTexture(glUnit);
		checkError();
		_priv::s.textureUnit = unit;
		changeUnit = true;
	}
	if (changeUnit || _priv::s.textureHandle != handle) {
		_priv::s.textureHandle = handle;
		glBindTexture(_priv::TextureTypes[std::enum_value(type)], handle);
		checkError();
		return true;
	}
	return false;
}

bool useProgram(Id handle) {
	if (_priv::s.programHandle == handle) {
		return false;
	}
	glUseProgram(handle);
	checkError();
	_priv::s.programHandle = handle;
	return true;
}

bool bindVertexArray(Id handle) {
	if (_priv::s.vertexArrayHandle == handle) {
		return false;
	}
	glBindVertexArray(handle);
	checkError();
	_priv::s.vertexArrayHandle = handle;
	return true;
}

bool bindBuffer(VertexBufferType type, Id handle) {
	const int typeIndex = std::enum_value(type);
	if (_priv::s.bufferHandle[typeIndex] == handle) {
		return false;
	}
	const GLenum glType = _priv::VertexBufferTypes[typeIndex];
	_priv::s.bufferHandle[typeIndex] = handle;
	glBindBuffer(glType, handle);
	checkError();
	return true;
}

bool bindBufferBase(VertexBufferType type, Id handle, uint32_t index) {
	const int typeIndex = std::enum_value(type);
	if (_priv::s.bufferBaseHandle[typeIndex] == handle) {
		return false;
	}
	const GLenum glType = _priv::VertexBufferTypes[typeIndex];
	_priv::s.bufferBaseHandle[typeIndex] = handle;
	glBindBufferBase(glType, (GLuint)index, handle);
	checkError();
	return true;
}

void genBuffers(uint8_t amount, Id* ids) {
	glGenBuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genBuffer() {
	Id id;
	genBuffers(1, &id);
	return id;
}

void deleteBuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteBuffers((GLsizei)amount, ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteBuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteBuffers(1, &id);
	id = InvalidId;
}

void genVertexArrays(uint8_t amount, Id* ids) {
	glGenVertexArrays((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genVertexArray() {
	Id id;
	genVertexArrays(1, &id);
	return id;
}

void deleteShader(Id& id) {
	if (id == InvalidId) {
		return;
	}
	glDeleteShader(id);
	checkError();
	id = InvalidId;
}

Id genShader(ShaderType type) {
	const GLenum glType = _priv::ShaderTypes[std::enum_value(type)];
	const Id id = glCreateShader(glType);
	checkError();
	return id;
}

void deleteProgram(Id& id) {
	if (id == InvalidId) {
		return;
	}
	glDeleteProgram(id);
	checkError();
	id = InvalidId;
}

Id genProgram() {
	Id id = glCreateProgram();
	checkError();
	return id;
}

void deleteVertexArrays(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteVertexArrays((GLsizei)amount, ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteVertexArray(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteVertexArrays(1, &id);
	id = InvalidId;
}

void genTextures(uint8_t amount, Id* ids) {
	glGenTextures((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genTexture() {
	Id id;
	genTextures(1, &id);
	return id;
}

void deleteTextures(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteTextures((GLsizei)amount, ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteTexture(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteTextures(1, &id);
	id = InvalidId;
}

void genFramebuffers(uint8_t amount, Id* ids) {
	glGenFramebuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genFramebuffer() {
	Id id;
	genFramebuffers(1, &id);
	return id;
}

void deleteFramebuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteFramebuffers((GLsizei)amount, ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteFramebuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteFramebuffers(1, &id);
	id = InvalidId;
}

void genRenderbuffers(uint8_t amount, Id* ids) {
	glGenRenderbuffers((GLsizei)amount, (GLuint*)ids);
	checkError();
}

Id genRenderbuffer() {
	Id id;
	genRenderbuffers(1, &id);
	return id;
}

void deleteRenderbuffers(uint8_t amount, Id* ids) {
	if (amount == 0) {
		return;
	}
	glDeleteRenderbuffers((GLsizei)amount, ids);
	checkError();
	for (int i = 0; i < amount; ++i) {
		ids[i] = InvalidId;
	}
}

void deleteRenderbuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteRenderbuffers(1, &id);
	id = InvalidId;
}

void configureAttribute(const Attribute& a) {
	glEnableVertexAttribArray(a.index);
	if (a.typeIsInt) {
		glVertexAttribIPointer(a.index, a.size, _priv::DataTypes[std::enum_value(a.type)], a.stride, GL_OFFSET_CAST(a.offset));
	} else {
		glVertexAttribPointer(a.index, a.size, _priv::DataTypes[std::enum_value(a.type)], a.normalized, a.stride, GL_OFFSET_CAST(a.offset));
	}
	if (a.divisor > 0) {
		glVertexAttribDivisor(a.index, a.divisor);
	}
	checkError();
}

Id bindFramebuffer(FrameBufferMode mode, Id handle, Id textureHandle) {
	const Id old = _priv::s.framebufferHandle;
	if (old == handle && _priv::s.framebufferTextureHandle == textureHandle) {
		return handle;
	}
	_priv::s.framebufferHandle = handle;
	_priv::s.framebufferTextureHandle = textureHandle;
	const int typeIndex = std::enum_value(mode);
	const GLenum glType = _priv::FrameBufferModes[typeIndex];
	glBindFramebuffer(glType, handle);
	if (textureHandle != InvalidId) {
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0);
	}
	checkError();
	return old;
}

bool bindRenderbuffer(Id handle) {
	glBindRenderbuffer(GL_RENDERBUFFER, handle);
	checkError();
	return true;
}

void bufferData(VertexBufferType type, VertexBufferMode mode, const void* data, size_t size) {
	glBufferData(_priv::VertexBufferTypes[std::enum_value(type)], (GLsizeiptr)size, data, _priv::VertexBufferModes[std::enum_value(mode)]);
	checkError();
}

void bufferSubData(VertexBufferType type, intptr_t offset, const void* data, size_t size) {
	glBufferSubData(_priv::VertexBufferTypes[std::enum_value(type)], (GLintptr)offset, (GLsizeiptr)size, data);
	checkError();
}

void disableDepthCompareTexture(TextureUnit unit, video::TextureType type, Id depthTexture) {
	bindTexture(unit, type, depthTexture);
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	checkError();
}

void setupDepthCompareTexture(TextureUnit unit, video::TextureType type, Id depthTexture) {
	bindTexture(unit, type, depthTexture);
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	glTexParameteri(glType, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(glType, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
	checkError();
}

bool setupFramebuffer(Id& _fbo, Id& _texture, Id& _depth, const glm::ivec2& dimension) {
	_fbo = genFramebuffer();
	bindFramebuffer(video::FrameBufferMode::Default, _fbo);
	_texture = genTexture();
	bindTexture(video::TextureUnit::Upload, TextureType::Texture2D, _texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, dimension.x, dimension.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _texture, 0);

	_depth = genRenderbuffer();
	bindRenderbuffer(_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, dimension.x, dimension.y);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depth);

	checkError();

	const GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE) {
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

	return true;
}

void setupTexture(video::TextureType type, video::TextureWrap wrap) {
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(glType, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(glType, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	const GLenum glWrap = _priv::TextureWraps[std::enum_value(wrap)];
	glTexParameteri(glType, GL_TEXTURE_WRAP_S, glWrap);
	glTexParameteri(glType, GL_TEXTURE_WRAP_T, glWrap);
	glTexParameteri(glType, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(glType, GL_TEXTURE_MAX_LEVEL, 0);
	checkError();
}

void uploadTexture(video::TextureType type, video::TextureFormat format, int width, int height, const uint8_t* data, int index) {
	const _priv::Formats& f = _priv::textureFormats[std::enum_value(format)];
	const GLenum glType = _priv::TextureTypes[std::enum_value(type)];
	if (type == TextureType::Texture2D) {
		glTexImage2D(glType, 0, f.internalFormat, width, height, 0, f.dataFormat, f.dataType, (const void*)data);
		checkError();
	} else {
		glTexImage3D(glType, 0, f.internalFormat, width, height, index, 0, f.dataFormat, f.dataType, (const void*)data);
		checkError();
	}
}

void drawElements(Primitive mode, size_t numIndices, DataType type) {
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	glDrawElements(glMode, (GLsizei)numIndices, _priv::DataTypes[std::enum_value(type)], nullptr);
	checkError();
}

void drawElementsInstanced(Primitive mode, size_t numIndices, DataType type, size_t amount) {
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	glDrawElementsInstanced(glMode, (GLsizei)numIndices, _priv::DataTypes[std::enum_value(type)], nullptr, (GLsizei)amount);
	checkError();
}

void drawElementsBaseVertex(Primitive mode, size_t numIndices, DataType type, size_t indexSize, int baseIndex, int baseVertex) {
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	glDrawElementsBaseVertex(glMode, (GLsizei)numIndices, _priv::DataTypes[std::enum_value(type)], GL_OFFSET_CAST(indexSize * baseIndex), (GLint)baseVertex);
	checkError();
}

void drawArrays(Primitive mode, size_t count) {
	const GLenum glMode = _priv::Primitives[std::enum_value(mode)];
	glDrawArrays(glMode, (GLint)0, (GLsizei)count);
	checkError();
}

void enableDebug(DebugSeverity severity) {
	GLDebug::enable(severity);
}


}
