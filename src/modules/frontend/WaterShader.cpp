/**
 * @file
 */

#include "WaterShader.h"

namespace frontend {

bool WaterShader::setup() {
	if (!loadProgram("shaders/water")) {
		return false;
	}

	checkAttributes({"a_pos", "a_info"});
	checkUniforms({"u_projection", "u_model", "u_view", "u_texture", "u_viewdistance", "u_fogrange", "u_materialcolor[0]"});
	return true;
}

}
