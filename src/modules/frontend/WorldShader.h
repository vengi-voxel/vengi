/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

#define WORLD_SHADER_ATTRIBUTES "a_pos", "a_info"
#define WORLD_SHADER_UNIFORMS "u_projection", "u_model", "u_view", "u_texture", "u_viewdistance", "u_fogrange", "u_materialcolor[0]"

class WorldShader : public video::Shader {
public:
	bool setup();
};

typedef std::shared_ptr<WorldShader> WorldShaderPtr;

}
