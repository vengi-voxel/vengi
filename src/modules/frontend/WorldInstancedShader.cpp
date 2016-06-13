/**
 * @file
 */

#include "WorldShader.h"
#include "WorldInstancedShader.h"

namespace frontend {

bool WorldInstancedShader::setup() {
	if (!loadProgram("shaders/world_instanced")) {
		return false;
	}

	checkAttributes({"a_offset", WORLD_SHADER_ATTRIBUTES});
	checkUniforms({WORLD_SHADER_UNIFORMS});
	return true;
}

}
