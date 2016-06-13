/**
 * @file
 */

#include "WorldShader.h"

namespace frontend {

bool WorldShader::setup() {
	if (!loadProgram("shaders/world")) {
		return false;
	}

	checkAttributes({WORLD_SHADER_ATTRIBUTES});
	checkUniforms({WORLD_SHADER_UNIFORMS});
	return true;
}

}
