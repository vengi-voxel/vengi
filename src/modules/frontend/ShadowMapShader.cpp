/**
 * @file
 */

#include "ShadowMapShader.h"

namespace frontend {

bool ShadowMapShader::setup() {
	if (!loadProgram("shaders/shadowmap")) {
		return false;
	}

	checkAttributes({"a_pos"});
	checkUniforms({"u_light", "u_model"});
	return true;
}

}
