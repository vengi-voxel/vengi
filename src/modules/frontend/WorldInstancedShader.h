/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class WorldInstancedShader : public video::Shader {
public:
	bool setup();
};

typedef std::shared_ptr<WorldInstancedShader> PlantShaderPtr;

}
