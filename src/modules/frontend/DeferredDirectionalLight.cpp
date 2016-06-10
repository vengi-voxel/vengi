/**
 * @file
 */

#include "DeferredDirectionalLight.h"

namespace frontend {

bool DeferredDirectionalLight::setup() {
	if (!loadProgram("shaders/deferred_light_dir")) {
		return false;
	}

	if (!hasAttribute("a_pos")) {
		Log::error("no attribute a_pos found");
	}
	if (!hasUniform("u_screensize")) {
		Log::error("no uniform u_screensize found");
	}
	if (!hasUniform("u_pos")) {
		Log::error("no uniform u_pos found");
	}
	if (!hasUniform("u_norm")) {
		Log::error("no uniform u_norm found");
	}
	if (!hasUniform("u_color")) {
		Log::error("no uniform u_color found");
	}
	return true;
}

}
