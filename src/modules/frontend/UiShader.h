/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class UiShader : public video::Shader {
public:
	bool setup() {
		if (!loadProgram("shaders/ui")) {
			return false;
		}
		// no attributes
		checkUniforms({"u_texture", "u_projection"});
		return true;
	}

	inline bool setTexture(int u_texture) const {
		if (!hasUniform("u_texture")) {
			return false;
		}
		setUniformi("u_texture", u_texture);
		return true;
	}

	inline bool setProjection(const glm::mat4& u_projection) const {
		if (!hasUniform("u_projection")) {
			return false;
		}
		setUniformMatrix("u_projection", u_projection);
		return true;
	}

};

typedef std::shared_ptr<UiShader> UiShaderPtr;

}
