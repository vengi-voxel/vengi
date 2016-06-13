/**
 * @file
 */

#include "WorldShader.h"

namespace frontend {

bool WorldShader::setup() {
	if (!loadProgram("shaders/world")) {
		return false;
	}

	checkAttributes({"a_pos", "a_info"});
	checkUniforms({"u_projection", "u_model", "u_view", "u_texture", "u_viewdistance", "u_fogrange", "u_materialcolor[0]"});
	return true;
}

}
