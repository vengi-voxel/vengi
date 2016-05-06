/**
 * @file
 */

#include "core/Log.h"
#include "GLFunc.h"

class GLDebug {
private:
	static bool _enabled;
#if defined(GL_ARB_debug_output)
	static void debugOutputCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam) {
		const char* typeStr;
		switch (type) {
		case GL_DEBUG_TYPE_ERROR_ARB:
			typeStr = "ERROR";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
			typeStr = "DEPRECATED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
			typeStr = "UNDEFINED_BEHAVIOR";
			break;
		case GL_DEBUG_TYPE_PORTABILITY_ARB:
			typeStr = "PORTABILITY";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE_ARB:
			typeStr = "PERFORMANCE";
			break;
		case GL_DEBUG_TYPE_OTHER_ARB:
			typeStr = "OTHER";
			break;
		default:
			typeStr = "<unknown>";
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
			break;
		default:
			sevStr = "<unknown>";
			break;
		}
		Log::warn("##### OpenGL Debug Message:\ntype: %s, id: %d, severity: %s\nmsg: %s\n", typeStr, id, sevStr, message);
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
		glDebugMessageControlARB(GL_DONT_CARE, GL_DONT_CARE, glSeverity, 0, nullptr, GL_TRUE);
		if (!_enabled) {
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
			glDebugMessageCallbackARB(debugOutputCallback, nullptr);
			GL_checkError();
			_enabled = true;
		}
		Log::info("enable opengl debug messages");
#else
		Log::warn("Not implemented");
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

