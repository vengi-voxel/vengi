/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class PlantShader : public video::Shader {
public:
	bool setup();
};

typedef std::shared_ptr<PlantShader> PlantShaderPtr;

}
