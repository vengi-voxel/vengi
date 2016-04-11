#pragma once

#include "video/Shader.h"

namespace frontend {

class TerrainShader : public video::Shader {
public:
	bool init();
};

typedef std::shared_ptr<TerrainShader> TerrainShaderPtr;

}
