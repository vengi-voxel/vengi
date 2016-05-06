/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class WorldShader : public video::Shader {
public:
	bool init();
};

typedef std::shared_ptr<WorldShader> WorldShaderPtr;

}
