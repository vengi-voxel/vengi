/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class MeshShader : public video::Shader {
public:
	bool setup() {
		if (!loadProgram("shaders/mesh")) {
			return false;
		}
		// no attributes
		checkUniforms({"u_texture", "u_projection", "u_model", "u_view", "u_fogrange", "u_viewdistance", "u_lightpos"});
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

	inline bool setModel(const glm::mat4& u_model) const {
		if (!hasUniform("u_model")) {
			return false;
		}
		setUniformMatrix("u_model", u_model);
		return true;
	}

	inline bool setView(const glm::mat4& u_view) const {
		if (!hasUniform("u_view")) {
			return false;
		}
		setUniformMatrix("u_view", u_view);
		return true;
	}

	inline bool setFogrange(float u_fogrange) const {
		if (!hasUniform("u_fogrange")) {
			return false;
		}
		setUniformf("u_fogrange", u_fogrange);
		return true;
	}

	inline bool setViewdistance(float u_viewdistance) const {
		if (!hasUniform("u_viewdistance")) {
			return false;
		}
		setUniformf("u_viewdistance", u_viewdistance);
		return true;
	}

	inline bool setLightpos(const glm::vec3& u_lightpos) const {
		if (!hasUniform("u_lightpos")) {
			return false;
		}
		setUniformVec3("u_lightpos", u_lightpos);
		return true;
	}

};

typedef std::shared_ptr<MeshShader> MeshShaderPtr;

}
