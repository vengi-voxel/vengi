/**
 * @file
 */

#include "video/Shader.h"
#include "video/UniformBuffer.h"
#include "core/Assert.h"
#include "core/GLM.h"
#include "flextVk.h"
#include <SDL.h>
#include <glm/gtc/type_ptr.hpp>

namespace video {

bool Shader::setAttributeLocation(const core::String& name, int location) {
	if (_program == InvalidId) {
		return false;
	}
	// TODO
	checkError();
	return true;
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

void Shader::setUniformui(int location, unsigned int value) const {
	if (checkUniformCache(location, &value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformi(int location, int value) const {
	if (checkUniformCache(location, &value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformi(int location, int value1, int value2) const {
	const int value[] = { value1, value2 };
	if (checkUniformCache(location, value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformi(int location, int value1, int value2, int value3) const {
	const int value[] = { value1, value2, value3 };
	if (checkUniformCache(location, value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformi(int location, int value1, int value2, int value3, int value4) const {
	const int value[] = { value1, value2, value3, value4 };
	if (checkUniformCache(location, value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform1iv(int location, const int* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(int))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform2iv(int location, const int* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(int))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform3iv(int location, const int* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(int))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformIVec2v(int location, const glm::ivec2* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::ivec2))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformIVec3v(int location, const glm::ivec3* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::ivec3))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformIVec4v(int location, const glm::ivec4* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::ivec4))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformf(int location, float value) const {
	if (checkUniformCache(location, &value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformf(int location, float value1, float value2) const {
	const float value[] = { value1, value2 };
	if (checkUniformCache(location, value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformf(int location, float value1, float value2, float value3) const {
	const float value[] = { value1, value2, value3 };
	if (checkUniformCache(location, value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformf(int location, float value1, float value2, float value3, float value4) const {
	const float value[] = { value1, value2, value3, value4 };
	if (checkUniformCache(location, value, sizeof(value))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform4fv(int location, const float* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(float))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformVec4v(int location, const glm::vec4* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::vec4))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformMatrixv(int location, const glm::mat4* matrixes, int amount, bool transpose) const {
	if (checkUniformCache(transpose ? -location : location, glm::value_ptr(matrixes[0]), amount * sizeof(glm::mat4))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformMatrixv(int location, const glm::mat3* matrixes, int amount, bool transpose) const {
	if (checkUniformCache(transpose ? -location : location, glm::value_ptr(matrixes[0]), amount * sizeof(glm::mat3))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform1fv(int location, const float* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(float))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform2fv(int location, const float* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(float))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformVec2v(int location, const glm::vec2* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::vec2))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniformVec3v(int location, const glm::vec3* value, int amount) const {
	if (checkUniformCache(location, glm::value_ptr(*value), amount * sizeof(glm::vec3))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setUniform3fv(int location, const float* values, int amount) const {
	if (checkUniformCache(location, values, amount * sizeof(float))) {
		// TODO
		checkError();
	}
	addUsedUniform(location);
}

void Shader::setVertexAttribute(int location, int size, DataType type, bool normalize, int stride, const void* buffer) const {
	core_assert_msg(getAttributeComponents(location) == -1 || getAttributeComponents(location) == size, "%i expected, but got %i components", getAttributeComponents(location), size);
	// TODO
	checkError();
}

void Shader::setVertexAttributeInt(int location, int size, DataType type, int stride, const void* buffer) const {
	core_assert_msg(getAttributeComponents(location) == -1 || getAttributeComponents(location) == size, "%i expected, but got %i components", getAttributeComponents(location), size);
	// TODO
	checkError();
}

bool Shader::enableVertexAttributeArray(int location) const {
	if (location < 0) {
		return false;
	}
	// TODO
	checkError();
	return true;
}

void Shader::disableVertexAttribute(int location) const {
	core_assert(location != -1);
	// TODO
	checkError();
}

bool Shader::setDivisor(int location, uint32_t divisor) const {
	if (location == -1) {
		return false;
	}
	// TODO
	checkError();
	return true;
}

}
