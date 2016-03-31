#include "WorldShader.h"

namespace frontend {

bool WorldShader::init() {
	if (!loadProgram("shaders/world")) {
		return false;
	}

	if (!hasAttribute("a_pos")) {
		Log::error("no attribute a_pos found");
		return false;
	}
	if (!hasAttribute("a_materialdensity")) {
		Log::error("no attribute a_materialdensity found");
		return false;
	}
	if (!hasUniform("u_projection")) {
		Log::error("no uniform u_projection found");
		return false;
	}
	if (!hasUniform("u_model")) {
		Log::error("no uniform u_model found");
		return false;
	}
	if (!hasUniform("u_view")) {
		Log::error("no uniform u_view found");
		return false;
	}
	if (!hasUniform("u_texture")) {
		Log::error("no uniform u_texture found");
		return false;
	}
	if (!hasUniform("u_viewdistance")) {
		Log::error("no uniform u_viewdistance found");
		return false;
	}
	if (!hasUniform("u_fogrange")) {
		Log::error("no uniform u_fogrange found");
		return false;
	}

	return true;
}

}
