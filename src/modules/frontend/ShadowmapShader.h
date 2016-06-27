/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class ShadowmapShader : public video::Shader {
public:
	bool setup() {
		if (!loadProgram("shaders/shadowmap")) {
			return false;
		}
		// no attributes
		checkUniforms({"u_light"});
		return true;
	}

	inline bool setLight(const glm::mat4& u_light) const {
		if (!hasUniform("u_light")) {
			return false;
		}
		setUniformMatrix("u_light", u_light);
		return true;
	}

};

typedef std::shared_ptr<ShadowmapShader> ShadowmapShaderPtr;

}
