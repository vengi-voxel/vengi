/**
 * @file
 */

#pragma once

#include "video/Shader.h"

namespace frontend {

class DeferredDirectionalLight : public video::Shader {
public:
	bool setup();
};

typedef std::shared_ptr<DeferredDirectionalLight> DeferredDirectionalLightPtr;

}
