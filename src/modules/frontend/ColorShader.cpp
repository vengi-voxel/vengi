/**
 * @file
 */

#include "ColorShader.h"

namespace frontend {

bool ColorShader::init() {
	if (!loadProgram("shaders/color")) {
		return false;
	}

	if (!hasAttribute("a_pos")) {
		Log::error("no attribute a_pos found");
	}
	if (!hasAttribute("a_color")) {
		Log::error("no attribute a_color found");
	}
	if (!hasUniform("u_projection")) {
		Log::error("no uniform u_projection found");
	}
	if (!hasUniform("u_view")) {
		Log::error("no uniform u_view found");
	}

	return true;
}

}
