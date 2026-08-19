/**
 * @file
 */

#include "BloomRenderer.h"
#include "ConvolutionShaderConstants.h"
#include "ConvolutionShader.h"
#include "TextureShader.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ArrayLength.h"
#include "core/Trace.h"
#include "video/FrameBufferConfig.h"
#include "video/Renderer.h"
#include "video/ScopedBlendMode.h"
#include "video/ScopedFrameBuffer.h"
#include "video/ScopedState.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "video/Types.h"

namespace render {

BloomRenderer::BloomRenderer()
	: _convolutionShader(shader::ConvolutionShader::getInstance()),
	  _textureShader(shader::TextureShader::getInstance()), _combine2Shader(shader::Combine2Shader::getInstance()) {
}

bool BloomRenderer::init(bool yFlipped, int width, int height) {
	if (!_convolutionShader.setup()) {
		Log::error("Failed to init the convolution shader");
		return false;
	}
	if (!_textureShader.setup()) {
		Log::error("Failed to init the texture shader");
		return false;
	}
	if (!_combine2Shader.setup()) {
		Log::error("Failed to init the combine2 shader");
		return false;
	}

	resize(width, height);

	_black = video::createEmptyTexture("**black**");
	_yFlipped = yFlipped;
	_uvBufferReady = false;
	_bufferIndex = _vbo.createFullscreenQuad();
	_texBufferIndex = _vbo.create();
	ensureUvBuffer();

	core_assert_always(_convolutionData.create(_convolutionFragData));

	core_assert(_convolutionShader.getLocationPos() == _textureShader.getLocationPos());
	core_assert(_convolutionShader.getLocationTexcoord() == _textureShader.getLocationTexcoord());
	core_assert(_convolutionShader.getLocationPos() == _combine2Shader.getLocationPos());
	core_assert(_convolutionShader.getLocationTexcoord() == _combine2Shader.getLocationTexcoord());
	core_assert_always(_vbo.addAttribute(_combine2Shader.getPosAttribute(_bufferIndex, &glm::vec2::x)));
	core_assert_always(_vbo.addAttribute(_combine2Shader.getTexcoordAttribute(_texBufferIndex, &glm::vec2::x)));

	_bloomPasses = core::findVar(cfg::ClientBloomPasses);
	return true;
}

void BloomRenderer::ensureUvBuffer() {
	if (_uvBufferReady) {
		return;
	}
	if (_yFlipped) {
		_vbo.createFullscreenTextureBufferYFlipped(_texBufferIndex);
	} else {
		_vbo.createFullscreenTextureBuffer(_texBufferIndex);
	}
	_uvBufferReady = true;
}

int BloomRenderer::activePasses() const {
	int n = _bloomPasses != nullptr ? _bloomPasses->intVal() : passes();
	if (n < 1) {
		n = 1;
	} else if (n > passes()) {
		n = passes();
	}
	return n;
}

bool BloomRenderer::resize(int width, int height) {
	video::TextureConfig tcfg = video::createDefaultTextureConfig();
	tcfg.filterMin(video::TextureFilter::Nearest);
	tcfg.filterMag(video::TextureFilter::Linear);
	tcfg.borderColor(glm::vec4(0.0f));

	if (width <= (1 << passes()) || height <= (1 << passes())) {
		Log::debug("Given width and height of the bloom renderer is not enough: %i:%i", width, height);
	}

	video::FrameBufferConfig bloomCfg;
	bloomCfg.dimension(glm::ivec2(width, height));
	bloomCfg.addTextureAttachment(tcfg, video::FrameBufferAttachment::Color0);

	for (int i = 0; i < lengthof(_bloom); ++i) {
		if (!_bloom[i].init(bloomCfg)) {
			Log::error("Failed to init the bloom framebuffer %i", i);
			return false;
		}
	}

	for (int i = 0; i < passes(); ++i) {
		const int h = core_max(1, height / (1 << (i + 1)));
		const int w = core_max(1, width / (1 << (i + 1)));
		video::FrameBufferConfig cfg;
		cfg.dimension(glm::ivec2(w, h));
		cfg.addTextureAttachment(tcfg, video::FrameBufferAttachment::Color0);
		_frameBuffers0[i].shutdown();
		_frameBuffers1[i].shutdown();
		_frameBuffers2[i].shutdown();
		if (!_frameBuffers0[i].init(cfg) || !_frameBuffers1[i].init(cfg) || !_frameBuffers2[i].init(cfg)) {
			Log::error("Failed to init the downsampling framebuffer %i", i);
			return false;
		}
	}
	_lastBlurWidth = -1;
	_lastBlurHorizontal = -1;
	return true;
}

void BloomRenderer::blur(const video::TexturePtr &source, video::FrameBuffer &dest, bool horizontal) {
	constexpr int filterSize = shader::ConvolutionShaderConstants::getFilterSize();
	const float halfWidth = (float)(filterSize - 1) * 0.5f;
	const int width = source->width();
	const int horizKey = horizontal ? 1 : 0;
	if (width != _lastBlurWidth || horizKey != _lastBlurHorizontal) {
		const float offset = 1.2f / (float)width;
		const float x = horizontal ? offset : 0.0f;
		{
			const float y0 = (float)0.0f - halfWidth;
			const float z0 = x * y0;
			_convolutionFragData.offsets0.x = offset * y0 - z0;
			_convolutionFragData.offsets0.y = z0;
		}
		{
			const float y1 = (float)1.0f - halfWidth;
			const float z1 = x * y1;
			_convolutionFragData.offsets1.x = offset * y1 - z1;
			_convolutionFragData.offsets1.y = z1;
		}
		{
			const float y2 = (float)2.0f - halfWidth;
			const float z2 = x * y2;
			_convolutionFragData.offsets2.x = offset * y2 - z2;
			_convolutionFragData.offsets2.y = z2;
		}
		_convolutionFragData.coefficients0 = 0.25f;
		_convolutionFragData.coefficients1 = 0.5f;
		_convolutionFragData.coefficients2 = 0.25f;
		core_assert_always(_convolutionData.update(_convolutionFragData));
		_lastBlurWidth = width;
		_lastBlurHorizontal = horizKey;
	}
	core_assert_always(_convolutionShader.setConv(_convolutionData.getConvUniformBuffer()));
	core_assert_always(video::bindTexture(_convolutionShader.getBoundImageTexUnit(), source));
	dest.bind(true);
	video::viewport(0, 0, source->width(), source->height());
	video::statsFullscreenPass();
	video::drawArrays(video::Primitive::Triangles, 6);
}

void BloomRenderer::apply(video::FrameBuffer *sources, video::FrameBuffer *dests, int passCount) {
	for (int i = 0; i < passCount; i++) {
		const int l = passCount - i - 1;
		{
			video::ScopedShader scoped(_combine2Shader);
			dests[l].bind(true);
			const video::TexturePtr &srcTex = sources[l].texture();
			if (i != 0) {
				const video::TexturePtr &destTex = dests[l + 1].texture();
				core_assert_always(video::bindTexture(video::TextureUnit::One, destTex));
			} else {
				core_assert_always(video::bindTexture(video::TextureUnit::One, _black));
			}
			core_assert_always(video::bindTexture(video::TextureUnit::Zero, srcTex));
			video::statsFullscreenPass();
			video::drawArrays(video::Primitive::Triangles, 6);
		}
		{
			video::ScopedShader scoped(_convolutionShader);
			blur(dests[l].texture(), sources[l], true);
			blur(sources[l].texture(), dests[l], false);
		}
	}
}

void BloomRenderer::render(const video::TexturePtr& srcTexture, const video::TexturePtr& glowTexture) {
	core_trace_scoped(BloomRender);
	video::ScopedState depthTest(video::State::DepthTest, false);
	video::ScopedState scissor(video::State::Scissor, false);
	video::ScopedState cullFace(video::State::CullFace, false);
	video::ScopedBlendMode blendMode(video::BlendMode::One, video::BlendMode::OneMinusSourceAlpha);

	// backup the current state
	video::Id oldFB = video::currentFramebuffer();
	int viewport[4];
	video::getViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	ensureUvBuffer();
	const int passCount = activePasses();

	{
		video::ScopedShader scoped(_convolutionShader);
		core_assert_always(_vbo.bind());
		blur(glowTexture, _bloom[0], false);
		blur(_bloom[0].texture(), _bloom[1], true);
	}

	// prepare the first source buffer by rendering the glow texture into it.
	{
		video::ScopedShader scoped(_textureShader);
		_frameBuffers0[0].bind(true);
		video::bindTexture(video::TextureUnit::Zero, _bloom[1].texture());
		video::statsFullscreenPass();
		video::drawArrays(video::Primitive::Triangles, 6);
	}

	for (int i = 1; i < passCount; ++i) {
		{
			video::ScopedShader scoped(_convolutionShader);
			blur(_frameBuffers0[i - 1].texture(), _frameBuffers1[i - 1], false);
			blur(_frameBuffers1[i - 1].texture(), _frameBuffers2[i - 1], true);
		}
		{
			video::ScopedShader scoped(_textureShader);
			// blit into the next source buffer
			_frameBuffers0[i].bind(true);
			// use the texture from the final blur stage
			const video::TexturePtr &blurTex = _frameBuffers2[i - 1].texture();
			core_assert_always(video::bindTexture(video::TextureUnit::Zero, blurTex));
			video::statsFullscreenPass();
			video::drawArrays(video::Primitive::Triangles, 6);
		}
	}

	apply(_frameBuffers0, _frameBuffers1, passCount);

	{
		video::ScopedShader scoped(_combine2Shader);
		_bloom[0].bind(true);
		video::bindTexture(video::TextureUnit::Zero, glowTexture);
		video::bindTexture(video::TextureUnit::One, _frameBuffers1[0].texture());
		video::statsFullscreenPass();
		video::drawArrays(video::Primitive::Triangles, 6);
	}

	{
		// Copy the scene out of the destination FBO, then combine scene+bloom
		// back in. Sampling a color attachment while rendering into it is
		// invalid on Vulkan; the blit path is valid on OpenGL too.
		const int w = srcTexture->width();
		const int h = srcTexture->height();
		video::blitFramebuffer(oldFB, _bloom[1].handle(), video::ClearFlag::Color, w, h);
		video::bindFramebuffer(oldFB);
		video::viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
		{
			const video::FrameBufferAttachment color0[] = {video::FrameBufferAttachment::Color0};
			const video::FrameBufferAttachment color01[] = {video::FrameBufferAttachment::Color0,
															video::FrameBufferAttachment::Color1};
			video::drawBuffers(1, color0);
			video::ScopedShader scoped(_combine2Shader);
			video::bindTexture(video::TextureUnit::Zero, _bloom[1].texture());
			video::bindTexture(video::TextureUnit::One, _bloom[0].texture());
			video::statsFullscreenPass();
			video::drawArrays(video::Primitive::Triangles, 6);
			video::drawBuffers(2, color01);
		}
		_vbo.unbind();
	}
}

video::TexturePtr BloomRenderer::texture() const {
	return texture1(0);
}

video::TexturePtr BloomRenderer::texture0(int pass) const {
	core_assert(pass >= 0 && pass < passes());
	return _frameBuffers0[pass].texture(video::FrameBufferAttachment::Color0);
}

video::TexturePtr BloomRenderer::texture1(int pass) const {
	core_assert(pass >= 0 && pass < passes());
	return _frameBuffers1[pass].texture(video::FrameBufferAttachment::Color0);
}

video::TexturePtr BloomRenderer::texture2(int pass) const {
	core_assert(pass >= 0 && pass < passes());
	return _frameBuffers2[pass].texture(video::FrameBufferAttachment::Color0);
}

void BloomRenderer::shutdown() {
	for (int i = 0; i < passes(); ++i) {
		_frameBuffers0[i].shutdown();
		_frameBuffers1[i].shutdown();
		_frameBuffers2[i].shutdown();
	}
	for (int i = 0; i < lengthof(_bloom); ++i) {
		_bloom[i].shutdown();
	}
	_convolutionData.shutdown();
	_convolutionShader.shutdown();
	_textureShader.shutdown();
	_combine2Shader.shutdown();
	if (_black) {
		_black->shutdown();
		_black = video::TexturePtr();
	}
	_vbo.shutdown();
	_uvBufferReady = false;
	_bloomPasses = core::VarPtr();
}

} // namespace render
