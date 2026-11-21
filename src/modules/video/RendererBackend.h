/**
 * @file
 */

#pragma once

#include "core/Var.h"
#include "video/RendererInterface.h"

namespace video {
namespace _priv {

class RendererImpl {
public:
	virtual ~RendererImpl() = default;
	virtual void setup() = 0;

#define RENDERER_DEFAULT(value)
#define RENDERER_METHOD(result, name, parameters, arguments) virtual result name parameters = 0;
#include "RendererBackendMethods.inl"
#undef RENDERER_METHOD
#undef RENDERER_DEFAULT
};

#define RENDERER_DEFAULT(value) = value
#define RENDERER_METHOD(result, name, parameters, arguments) result name parameters override;
#ifdef USE_GL_RENDERER
class GLRenderer final : public RendererImpl {
public:
	void setup() override;

#include "RendererBackendMethods.inl"
};
#endif

#ifdef USE_VK_RENDERER
class VkRenderer final : public RendererImpl {
public:
	void setup() override;

#include "RendererBackendMethods.inl"
};
#endif
#undef RENDERER_METHOD
#undef RENDERER_DEFAULT

core::VarPtr registerRendererVar();

} // namespace _priv
} // namespace video
