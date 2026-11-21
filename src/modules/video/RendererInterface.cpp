/**
 * @file
 */

#include "video/RendererBackend.h"
#include "app/I18N.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "video/Renderer.h"
#include "video/Shader.h"

namespace video {

namespace {

#ifdef USE_GL_RENDERER
_priv::GLRenderer glRenderer;
#endif
#ifdef USE_VK_RENDERER
_priv::VkRenderer vkRenderer;
#endif

#ifdef USE_GL_RENDERER
constexpr const char *DefaultRenderer = "opengl";
_priv::RendererImpl *_rendererImpl = &glRenderer;
#else
constexpr const char *DefaultRenderer = "vulkan";
_priv::RendererImpl *_rendererImpl = &vkRenderer;
#endif

} // namespace

core::VarPtr _priv::registerRendererVar() {
	const core::VarDef renderer(cfg::RenderBackend, DefaultRenderer, N_("Renderer"),
							   N_("Renderer backend at startup: opengl or vulkan"), core::CV_READONLY);
	return core::Var::registerVar(renderer);
}

bool isVulkanRenderer() {
#ifdef USE_VK_RENDERER
	return _rendererImpl == &vkRenderer;
#else
	return false;
#endif
}

void setup() {
	const core::String &requested = _priv::registerRendererVar()->strVal();
#ifdef USE_GL_RENDERER
	_rendererImpl = &glRenderer;
#else
	_rendererImpl = &vkRenderer;
#endif

#ifdef USE_VK_RENDERER
	if (requested == "vulkan") {
		_rendererImpl = &vkRenderer;
	}
#endif
	const char *selected = isVulkanRenderer() ? "vulkan" : "opengl";
	if (requested != selected) {
		Log::warn("Renderer '%s' unavailable in this build; using %s", requested.c_str(), selected);
	}
	Log::info("Renderer backend: %s", selected);
	Shader::vulkan = isVulkanRenderer();
	renderState().clipDepthZeroToOne = isVulkanRenderer();
	renderState().clipOriginLowerLeft = !isVulkanRenderer();
	_rendererImpl->setup();
}

#define RENDERER_DEFAULT(value)
#define RENDERER_METHOD(result, name, parameters, arguments) \
	result name parameters { return _rendererImpl->name arguments; }
#include "RendererBackendMethods.inl"
#undef RENDERER_METHOD
#undef RENDERER_DEFAULT

} // namespace video
