/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class DeferredLightDirShader : public video::Shader {
public:
	bool setup() {
		if (!loadProgram("shaders/deferred_light_dir")) {
			return false;
		}
		// no attributes
		checkUniforms({"u_screensize", "u_pos", "u_color", "u_norm", "u_lightpos", "u_diffuse_color"});
		return true;
	}

	inline bool setScreensize(const glm::vec2& u_screensize) const {
		if (!hasUniform("u_screensize")) {
			return false;
		}
		setUniformVec2("u_screensize", u_screensize);
		return true;
	}

	inline bool setPos(int u_pos) const {
		if (!hasUniform("u_pos")) {
			return false;
		}
		setUniformi("u_pos", u_pos);
		return true;
	}

	inline bool setColor(int u_color) const {
		if (!hasUniform("u_color")) {
			return false;
		}
		setUniformi("u_color", u_color);
		return true;
	}

	inline bool setNorm(int u_norm) const {
		if (!hasUniform("u_norm")) {
			return false;
		}
		setUniformi("u_norm", u_norm);
		return true;
	}

	inline bool setLightpos(const glm::vec3& u_lightpos) const {
		if (!hasUniform("u_lightpos")) {
			return false;
		}
		setUniformVec3("u_lightpos", u_lightpos);
		return true;
	}

	inline bool setDiffuseColor(const glm::vec3& u_diffuse_color) const {
		if (!hasUniform("u_diffuse_color")) {
			return false;
		}
		setUniformVec3("u_diffuse_color", u_diffuse_color);
		return true;
	}

};

typedef std::shared_ptr<DeferredLightDirShader> DeferredLightDirShaderPtr;

}
