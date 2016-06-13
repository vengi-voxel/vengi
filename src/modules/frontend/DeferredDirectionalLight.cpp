/**
 * @file
 */

#include "DeferredDirectionalLight.h"

namespace frontend {

bool DeferredDirectionalLight::setup() {
	if (!loadProgram("shaders/deferred_light_dir")) {
		return false;
	}

	checkAttributes({"a_pos"});
	checkUniforms({"u_screensize", "u_pos", "u_norm", "u_color"});
	return true;
}

}
