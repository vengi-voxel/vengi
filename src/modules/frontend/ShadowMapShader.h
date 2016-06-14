/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class ShadowMapShader : public video::Shader {
public:
	bool setup();
};

typedef std::shared_ptr<ShadowMapShader> ShadowMapShaderPtr;

}
