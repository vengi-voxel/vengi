/**
 * @file
 */

#include "WorldInstancedShader.h"

namespace frontend {

bool WorldInstancedShader::setup() {
	if (!loadProgram("shaders/world_instanced")) {
		return false;
	}

	checkAttributes({"a_pos", "a_offset", "a_info"});
	checkUniforms({"u_projection", "u_model", "u_view", "u_texture", "u_viewdistance", "u_fogrange", "u_materialcolor[0]"});
	return true;
}

}
