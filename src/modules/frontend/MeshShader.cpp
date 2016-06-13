/**
 * @file
 */

#include "MeshShader.h"

namespace frontend {

bool MeshShader::setup() {
	if (!loadProgram("shaders/mesh")) {
		return false;
	}

	checkAttributes({"a_pos", "a_texcoords", "a_norm"});
	checkUniforms({"u_projection", "u_model", "u_view", "u_texture", "u_viewdistance", "u_fogrange"});
	return true;
}

}
