/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class WaterShader : public video::Shader {
public:
	bool setup();
};

typedef std::shared_ptr<WaterShader> WaterShaderPtr;

}
