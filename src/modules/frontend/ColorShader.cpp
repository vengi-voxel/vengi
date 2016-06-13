/**
 * @file
 */

#include "ColorShader.h"

namespace frontend {

bool ColorShader::setup() {
	if (!loadProgram("shaders/color")) {
		return false;
	}

	checkAttributes({"a_pos", "a_color"});
	checkUniforms({"u_projection", "u_view"});
	return true;
}

}
