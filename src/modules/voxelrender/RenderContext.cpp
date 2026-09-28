/**
 * @file
 */

#include "RenderContext.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Var.h"
#include "video/FrameBufferConfig.h"
#include "video/Renderer.h"
#include "video/TextureConfig.h"
#include "video/Types.h"

namespace voxelrender {

bool RenderContext::isEditMode() const {
	return renderMode == RenderMode::Edit;
}

bool RenderContext::isSceneMode() const {
	return renderMode == RenderMode::Scene;
}

bool RenderContext::applyTransforms() const {
	return isSceneMode() || applyTransformsInEditMode;
}

bool RenderContext::showCameras() const {
	return isSceneMode();
}

bool RenderContext::hasOit() const {
	return oitFrameBuffer.handle() != video::InvalidId;
}

static void addDepthBuffer(video::FrameBufferConfig &cfg) {
	cfg.depthBufferFormat(video::TextureFormat::D32F);
	cfg.stencilBuffer(false);
	cfg.depthBuffer(true);
}

static bool initOitFrameBuffer(video::FrameBuffer &oitFrameBuffer, const glm::ivec2 &size) {
	oitFrameBuffer.shutdown();
	if (size.x <= 0 || size.y <= 0) {
		return false;
	}

	video::TextureConfig colorCfg = video::createDefaultTextureConfig();
	colorCfg.format(video::TextureFormat::RGBA16F);
	colorCfg.filter(video::TextureFilter::Nearest);

	video::FrameBufferConfig cfg;
	cfg.dimension(size);
	cfg.samples(0);
	cfg.addTextureAttachment(colorCfg, video::FrameBufferAttachment::Color0);
	cfg.addTextureAttachment(colorCfg, video::FrameBufferAttachment::Color1);
	addDepthBuffer(cfg);

	if (!oitFrameBuffer.init(cfg)) {
		Log::warn("Failed to initialize weighted OIT framebuffer");
		oitFrameBuffer.shutdown();
		return false;
	}
	return true;
}

bool RenderContext::init(const glm::ivec2 &size) {
	video::FrameBufferConfig cfg;
	cfg.dimension(size);

	// Configure multisampling based on configuration variables
	const core::VarPtr &multisampleSamplesVar = core::getVar(cfg::ClientMultiSampleSamples);
	enableMultisampling = multisampleSamplesVar->intVal() > 1;
	multisampleSamples = multisampleSamplesVar->intVal();

	if (enableMultisampling && multisampleSamples > 1) {
		// Clamp to supported range
		const int maxSamples = video::limiti(video::Limit::MaxSamples);
		Log::debug("Hardware supports up to %d multisampling samples, requested: %d", maxSamples, multisampleSamples);
		multisampleSamples = glm::clamp(multisampleSamples, 2, maxSamples);

		// Ensure it's a power of 2 (common requirement)
		if ((multisampleSamples & (multisampleSamples - 1)) != 0) {
			// Find the next lower power of 2
			int powerOf2 = 2;
			while (powerOf2 * 2 <= multisampleSamples) {
				powerOf2 *= 2;
			}
			multisampleSamples = powerOf2;
			Log::debug("Adjusted to power of 2: %d samples", multisampleSamples);
		}

		Log::debug("Initializing volume renderer framebuffer with %d multisampling samples", multisampleSamples);
	} else {
		enableMultisampling = false;
		multisampleSamples = 0;
	}
	// Configure multisampling for the entire framebuffer (affects depth buffer)
	if (enableMultisampling) {
		cfg.samples(multisampleSamples);
	}

	// Add texture attachments
	if (enableMultisampling) {
		video::TextureConfig msaaConfig = video::createDefaultMultiSampleTextureConfig();
		msaaConfig.samples(multisampleSamples); // Set the actual sample count
		Log::debug("MSAA texture config - type: %d, samples: %d, format: %d", (int)msaaConfig.type(),
				   msaaConfig.samples(), (int)msaaConfig.format());
		cfg.addTextureAttachment(msaaConfig, video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(msaaConfig, video::FrameBufferAttachment::Color1); // bloom (also MSAA for consistency)

		// Add multisampled depth buffer
		video::TextureConfig msaaDepthConfig = video::createDefaultMultiSampleTextureConfig();
		msaaDepthConfig.samples(multisampleSamples);
		msaaDepthConfig.format(video::TextureFormat::D32F);
		cfg.addTextureAttachment(msaaDepthConfig, video::FrameBufferAttachment::Depth);
	} else {
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
		addDepthBuffer(cfg);
	}

	if (!frameBuffer.init(cfg)) {
		if (enableMultisampling) {
			Log::warn("Failed to initialize multisampled framebuffer, retrying without multisampling");
			// Retry without multisampling - first shutdown the failed framebuffer
			frameBuffer.shutdown();
			enableMultisampling = false;
			multisampleSamples = 0;
			video::FrameBufferConfig fallbackCfg;
			fallbackCfg.dimension(size);
			fallbackCfg.samples(0); // Explicitly disable multisampling
			fallbackCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0);
			fallbackCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1);
			addDepthBuffer(fallbackCfg);
			if (!frameBuffer.init(fallbackCfg)) {
				Log::error("Failed to initialize the volume renderer framebuffer");
				return false;
			}
		} else {
			Log::error("Failed to initialize the volume renderer framebuffer");
			return false;
		}
	}

	// If multisampling is enabled, create a resolve framebuffer with regular textures
	if (enableMultisampling) {
		video::FrameBufferConfig resolveCfg;
		resolveCfg.dimension(size);
		resolveCfg.samples(0); // No multisampling for resolve target
		resolveCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0);
		resolveCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1);
		addDepthBuffer(resolveCfg);

		if (!resolveFrameBuffer.init(resolveCfg)) {
			Log::error("Failed to initialize resolve framebuffer for multisampling");
			return false;
		}
		Log::debug("Successfully created resolve framebuffer for multisampling");
	}

	const glm::vec4 &fbUv = video::framebufferUV();
	if (!bloomRenderer.init(fbUv.y > fbUv.w, size.x, size.y)) {
		Log::error("Failed to initialize the bloom renderer");
		return false;
	}
	if (!initOitFrameBuffer(oitFrameBuffer, size)) {
		Log::warn("Weighted OIT disabled: falling back to sorted transparency");
	}
	return true;
}

bool RenderContext::resize(const glm::ivec2 &size) {
	if (frameBuffer.dimension() == size) {
		return true;
	}
	frameBuffer.shutdown();
	video::FrameBufferConfig cfg;
	cfg.dimension(size);

	// Configure multisampling for the entire framebuffer (affects depth buffer)
	if (enableMultisampling) {
		cfg.samples(multisampleSamples);
	}

	// Add texture attachments
	if (enableMultisampling) {
		video::TextureConfig msaaConfig = video::createDefaultMultiSampleTextureConfig();
		msaaConfig.samples(multisampleSamples); // Set the actual sample count
		Log::info("MSAA resize: texture config - type: %d, samples: %d, format: %d", (int)msaaConfig.type(),
				  msaaConfig.samples(), (int)msaaConfig.format());
		cfg.addTextureAttachment(msaaConfig, video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(msaaConfig, video::FrameBufferAttachment::Color1); // bloom (also MSAA for consistency)

		// Add multisampled depth buffer
		video::TextureConfig msaaDepthConfig = video::createDefaultMultiSampleTextureConfig();
		msaaDepthConfig.samples(multisampleSamples);
		msaaDepthConfig.format(video::TextureFormat::D32F);
		cfg.addTextureAttachment(msaaDepthConfig, video::FrameBufferAttachment::Depth);
	} else {
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
		addDepthBuffer(cfg);
	}
	// Check GL state before framebuffer creation
	if (enableMultisampling) {
		int maxSamples = video::limiti(video::Limit::MaxSamples);
		Log::debug("Resize GL_MAX_SAMPLES: %d, requested: %d", maxSamples, multisampleSamples);
	}

	if (!frameBuffer.init(cfg)) {
		Log::error("Failed to initialize the volume renderer framebuffer - FB incomplete multisample detected");
		return false;
	}
	Log::debug("Successfully created %s framebuffer in resize", enableMultisampling ? "multisampled" : "regular");

	// If multisampling is enabled, create/resize resolve framebuffer with regular textures
	if (enableMultisampling) {
		resolveFrameBuffer.shutdown();
		video::FrameBufferConfig resolveCfg;
		resolveCfg.dimension(size);
		resolveCfg.samples(0); // No multisampling for resolve target
		resolveCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0);
		resolveCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1);
		addDepthBuffer(resolveCfg);

		if (!resolveFrameBuffer.init(resolveCfg)) {
			Log::error("Failed to initialize resolve framebuffer for multisampling");
			return false;
		}
		Log::debug("Successfully created resolve framebuffer in resize");
	}

	// we have to do an y-flip here due to the framebuffer handling
	if (!bloomRenderer.resize(size.x, size.y)) {
		Log::error("Failed to initialize the bloom renderer");
		return false;
	}
	if (!initOitFrameBuffer(oitFrameBuffer, size)) {
		Log::warn("Weighted OIT disabled: falling back to sorted transparency");
	}
	return true;
}

bool RenderContext::updateMultisampling() {
	const core::VarPtr &multisampleSamplesVar = core::getVar(cfg::ClientMultiSampleSamples);
	bool newEnableMultisampling = multisampleSamplesVar->intVal() > 1;
	int newMultisampleSamples = multisampleSamplesVar->intVal();

	if (enableMultisampling != newEnableMultisampling || multisampleSamples != newMultisampleSamples) {
		enableMultisampling = newEnableMultisampling;
		multisampleSamples = newMultisampleSamples;
		// Recreate the framebuffer with new multisampling settings
		const glm::ivec2 currentSize = frameBuffer.dimension();
		return resize(currentSize);
	}
	return true;
}

static void restoreColorDrawBuffers(video::Id handle) {
	if (handle == video::InvalidId) {
		return;
	}
	const video::Id prev = video::bindFramebuffer(handle);
	const video::FrameBufferAttachment color01[] = {video::FrameBufferAttachment::Color0,
													video::FrameBufferAttachment::Color1};
	video::drawBuffers(2, color01);
	video::bindFramebuffer(prev);
}

void RenderContext::resolveMultisampling() {
	if (!enableMultisampling) {
		return;
	}
	const glm::ivec2 &dim = frameBuffer.dimension();
	const video::Id src = frameBuffer.handle();
	const video::Id dst = resolveFrameBuffer.handle();
	video::blitFramebuffer(src, dst, video::FrameBufferAttachment::Color0, dim.x, dim.y);
	video::blitFramebuffer(src, dst, video::FrameBufferAttachment::Color1, dim.x, dim.y);
	restoreColorDrawBuffers(dst);
	restoreColorDrawBuffers(src);
	video::blitFramebuffer(src, dst, video::ClearFlag::Depth, dim.x, dim.y);
}

void RenderContext::applyBloom() {
	if (!enableBloom || !sceneHasGlow) {
		return;
	}
	video::FrameBuffer &display = enableMultisampling ? resolveFrameBuffer : frameBuffer;
	const video::TexturePtr &color0 = display.texture(video::FrameBufferAttachment::Color0);
	const video::TexturePtr &color1 = display.texture(video::FrameBufferAttachment::Color1);
	if (!color0 || !color1) {
		return;
	}
	const glm::ivec2 &dim = display.dimension();
	const video::Id prev = video::bindFramebuffer(display.handle());
	int viewport[4];
	video::getViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	video::viewport(0, 0, dim.x, dim.y);
	bloomRenderer.render(color0, color1);
	video::bindFramebuffer(prev);
	video::viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

void RenderContext::beginOverlays() {
	if (enableMultisampling) {
		frameBuffer.unbind();
		resolveFrameBuffer.bind(false);
	}
	const video::FrameBufferAttachment color0[] = {video::FrameBufferAttachment::Color0};
	video::drawBuffers(1, color0);
}

void RenderContext::endFrame() {
	if (video::currentFramebuffer() != video::InvalidId) {
		const video::FrameBufferAttachment color01[] = {video::FrameBufferAttachment::Color0,
														video::FrameBufferAttachment::Color1};
		video::drawBuffers(2, color01);
	}
	if (enableMultisampling) {
		resolveFrameBuffer.unbind();
	} else {
		frameBuffer.unbind();
	}
}

void RenderContext::shutdown() {
	frameBuffer.shutdown();
	resolveFrameBuffer.shutdown();
	oitFrameBuffer.shutdown();
	bloomRenderer.shutdown();
}

} // namespace voxelrender
