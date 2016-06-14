/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class ShadowMapRenderShader : public video::Shader {
public:
	bool setup();
};

typedef std::shared_ptr<ShadowMapRenderShader> ShadowMapRenderShaderPtr;

}
