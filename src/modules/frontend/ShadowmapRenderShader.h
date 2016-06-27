/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class ShadowmapRenderShader : public video::Shader {
public:
	bool setup() {
		if (!loadProgram("shaders/shadowmap_render")) {
			return false;
		}
		// no attributes
		checkUniforms({"u_shadowmap"});
		return true;
	}

	inline bool setShadowmap(int u_shadowmap) const {
		if (!hasUniform("u_shadowmap")) {
			return false;
		}
		setUniformi("u_shadowmap", u_shadowmap);
		return true;
	}

};

typedef std::shared_ptr<ShadowmapRenderShader> ShadowmapRenderShaderPtr;

}
