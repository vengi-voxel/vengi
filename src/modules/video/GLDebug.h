/**
 * @file
 */

#include "core/Log.h"
#include "GLFunc.h"

class GLDebug {
private:
	static int _recompileErrors;
	static bool _enabled;
#if defined(GL_ARB_debug_output)
	static void debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam) {
		if (id == 131218) {
			++_recompileErrors;
			if (_recompileErrors <= 5) {
				return;
			}
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
	enum Severity {
		High, Medium, Low,
	};

	static void enable(Severity s) {
#if defined(GL_ARB_debug_output)
		GLenum glSeverity = GL_DONT_CARE;
		switch (s) {
		case High:
			glSeverity = GL_DEBUG_SEVERITY_HIGH_ARB;
			break;
		case Medium:
			glSeverity = GL_DEBUG_SEVERITY_MEDIUM_ARB;
			break;
		case Low:
			glSeverity = GL_DEBUG_SEVERITY_LOW_ARB;
			break;
		}
		if (glDebugMessageControlARB != nullptr) {
			glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, glSeverity, 0, nullptr, GL_TRUE);
			if (!_enabled) {
				glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
				glDebugMessageCallbackARB(debugOutputCallback, nullptr);
				GL_checkError();
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
		GL_checkError();
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
