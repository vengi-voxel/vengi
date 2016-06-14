/**
 * @file
 */

#include "ShadowMapRenderShader.h"

namespace frontend {

bool ShadowMapRenderShader::setup() {
	if (!loadProgram("shaders/shadowmap_render")) {
		return false;
	}

	checkAttributes({"a_pos", "a_texcoord"});
	checkUniforms({"u_shadowmap"});
	return true;
}

}
