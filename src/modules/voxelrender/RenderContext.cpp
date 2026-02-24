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

bool RenderContext::init(const glm::ivec2 &size) {
	video::FrameBufferConfig cfg;
	cfg.dimension(size);

	// Configure multisampling based on configuration variables
	const core::VarPtr &multisampleSamplesVar = core::getVar(cfg::ClientMultiSampleSamples);
	const core::VarPtr &multisampleBuffersVar = core::getVar(cfg::ClientMultiSampleBuffers);
	enableMultisampling = multisampleSamplesVar->intVal() > 0 && multisampleBuffersVar->intVal() > 0;
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
		msaaDepthConfig.format(video::TextureFormat::D24S8);
		cfg.addTextureAttachment(msaaDepthConfig, video::FrameBufferAttachment::DepthStencil);
	} else {
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
		cfg.depthBuffer(true);
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
			fallbackCfg.depthBuffer(true);
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
		resolveCfg.depthBuffer(true);

		if (!resolveFrameBuffer.init(resolveCfg)) {
			Log::error("Failed to initialize resolve framebuffer for multisampling");
			return false;
		}
		Log::debug("Successfully created resolve framebuffer for multisampling");
	}

	// we have to do an y-flip here due to the framebuffer handling
	if (!bloomRenderer.init(true, size.x, size.y)) {
		Log::error("Failed to initialize the bloom renderer");
		return false;
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
		msaaDepthConfig.format(video::TextureFormat::D24S8);
		cfg.addTextureAttachment(msaaDepthConfig, video::FrameBufferAttachment::DepthStencil);
	} else {
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
		cfg.depthBuffer(true);
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
		resolveCfg.depthBuffer(true);

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
	return true;
}

bool RenderContext::updateMultisampling() {
	const core::VarPtr &multisampleSamplesVar = core::getVar(cfg::ClientMultiSampleSamples);
	const core::VarPtr &multisampleBuffersVar = core::getVar(cfg::ClientMultiSampleBuffers);
	bool newEnableMultisampling = multisampleSamplesVar->intVal() > 0 && multisampleBuffersVar->intVal() > 0;
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

void RenderContext::shutdown() {
	frameBuffer.shutdown();
	resolveFrameBuffer.shutdown();
	bloomRenderer.shutdown();
}

} // namespace voxelrender
