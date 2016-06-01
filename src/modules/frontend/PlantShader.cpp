/**
 * @file
 */

#include "PlantShader.h"

namespace frontend {

bool PlantShader::setup() {
	if (!loadProgram("shaders/plant")) {
		return false;
	}

	if (!hasAttribute("a_offset")) {
		Log::error("no attribute a_offset found");
	}
	if (!hasAttribute("a_pos")) {
		Log::error("no attribute a_pos found");
	}
	if (!hasAttribute("a_info")) {
		Log::error("no attribute a_info found");
	}
	if (!hasUniform("u_projection")) {
		Log::error("no uniform u_projection found");
	}
	if (!hasUniform("u_model")) {
		Log::error("no uniform u_model found");
	}
	if (!hasUniform("u_view")) {
		Log::error("no uniform u_view found");
	}
	if (!hasUniform("u_texture")) {
		Log::error("no uniform u_texture found");
	}
	if (!hasUniform("u_viewdistance")) {
		Log::error("no uniform u_viewdistance found");
	}
	if (!hasUniform("u_fogrange")) {
		Log::error("no uniform u_fogrange found");
	}
	if (!hasUniform("u_materialcolor[0]")) {
		Log::error("no uniform u_materialcolor found");
	}
	return true;
}

}
