#pragma once

#include "video/Shader.h"

namespace frontend {

class ColoredCubesShader : public video::Shader {
public:
	bool init();
};

typedef std::shared_ptr<ColoredCubesShader> ColoredCubesShaderPtr;

}
