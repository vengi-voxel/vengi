/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class MeshShader : public video::Shader {
public:
	bool init();
};

typedef std::shared_ptr<MeshShader> MeshShaderPtr;

}
