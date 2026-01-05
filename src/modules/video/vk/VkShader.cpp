/**
 * @file
 */

#include "video/Shader.h"
#include "video/UniformBuffer.h"
#include "core/Assert.h"
#include "core/GLM.h"
#include "flextVk.h"
#include "core/sdl/SDLSystem.h"
#include <glm/gtc/type_ptr.hpp>

namespace video {

int32_t Shader::getUniformBufferOffset(const char *name) {
	return false;
}

bool Shader::setUniformBuffer(const core::String& name, const UniformBuffer& buffer) {
	const Uniform* uniform = getUniform(name);
	if (uniform == nullptr) {
		return false;
	}
	if (!uniform->block) {
		return false;
	}

	// TODO
	checkError();
	addUsedUniform(uniform->location);
	return false;
}

void Shader::setUniformi(int location, int value) const {
	if (checkUniformCache(location, &value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

}
