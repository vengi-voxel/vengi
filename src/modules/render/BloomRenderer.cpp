/**
 * @file
 */

#include "BloomRenderer.h"
#include "ConvolutionShaderConstants.h"
#include "ConvolutionShader.h"
#include "TextureShader.h"
#include "core/Log.h"
#include "core/ArrayLength.h"
#include "video/Camera.h"
#include "video/FrameBufferConfig.h"
#include "video/Renderer.h"
#include "video/ScopedBlendMode.h"
#include "video/ScopedFrameBuffer.h"
#include "video/ScopedState.h"
#include "video/ScopedViewPort.h"
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
	_bufferIndex = _vbo.createFullscreenQuad();
	_texBufferIndex = _vbo.create();

	core_assert(_convolutionShader.getLocationPos() == _textureShader.getLocationPos());
	core_assert(_convolutionShader.getLocationTexcoord() == _textureShader.getLocationTexcoord());
	core_assert(_convolutionShader.getLocationPos() == _combine2Shader.getLocationPos());
	core_assert(_convolutionShader.getLocationTexcoord() == _combine2Shader.getLocationTexcoord());
	core_assert_always(_vbo.addAttribute(_combine2Shader.getPosAttribute(_bufferIndex, &glm::vec2::x)));
	core_assert_always(_vbo.addAttribute(_combine2Shader.getTexcoordAttribute(_texBufferIndex, &glm::vec2::x)));
	return true;
}

bool BloomRenderer::resize(int width, int height) {
	video::TextureConfig tcfg = video::createDefaultTextureConfig();
	tcfg.filterMin(video::TextureFilter::Nearest);
	tcfg.filterMag(video::TextureFilter::Linear);
	tcfg.borderColor(glm::vec4(0.0f));

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
		const int h = height / (1 << (i + 1));
		const int w = width / (1 << (i + 1));
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
	return true;
}

void BloomRenderer::blur(const video::TexturePtr &source, video::FrameBuffer &dest, bool horizontal) {
	constexpr int filterSize = shader::ConvolutionShaderConstants::getFilterSize();
	glm::vec2 offsets[filterSize];
	const float halfWidth = (float)(filterSize - 1) * 0.5f;
	const float offset = 1.2f / (float)source->width();
	const float x = horizontal ? offset : 0.0f;

	for (int i = 0; i < filterSize; i++) {
		const float y = (float)i - halfWidth;
		const float z = x * y;
		offsets[i].x = offset * y - z;
		offsets[i].y = z;
	}
	core_assert_always(_convolutionShader.setOffsets(offsets));
	core_assert_always(video::bindTexture(video::TextureUnit::Zero, source));
	dest.bind(true);
	video::viewport(0, 0, source->width(), source->height());
	video::drawArrays(video::Primitive::Triangles, 6);
}

void BloomRenderer::apply(video::FrameBuffer *sources, video::FrameBuffer *dests) {
	for (int i = 0; i < passes(); i++) {
		const int l = passes() - i - 1;
		{
			video::ScopedShader scoped(_combine2Shader);
			dests[l].bind(true);
			core_assert_always(_combine2Shader.setTexture0(video::TextureUnit::Zero));
			core_assert_always(_combine2Shader.setTexture1(video::TextureUnit::One));
			const video::TexturePtr &srcTex = sources[l].texture();
			if (i != 0) {
				const video::TexturePtr &destTex = dests[l + 1].texture();
				core_assert_always(video::bindTexture(video::TextureUnit::One, destTex));
			} else {
				core_assert_always(video::bindTexture(video::TextureUnit::One, _black));
			}
			core_assert_always(video::bindTexture(video::TextureUnit::Zero, srcTex));
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
	video::ScopedState depthTest(video::State::DepthTest, false);
	video::ScopedState scissor(video::State::Scissor, false);
	video::ScopedBlendMode blendMode(video::BlendMode::One, video::BlendMode::OneMinusSourceAlpha);

	// backup the current state
	video::Id oldFB = video::currentFramebuffer();
	int viewport[4];
	video::getViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

	_vbo.createFullscreenTextureBufferYFlipped(_texBufferIndex);

	{
		constexpr int filterSize = shader::ConvolutionShaderConstants::getFilterSize();
		static /* const */ float coeff[filterSize] = { // TODO: const
			0.25f, 0.5f, 0.25f
		};
		video::ScopedShader scoped(_convolutionShader);
		core_assert_always(_convolutionShader.setImage(video::TextureUnit::Zero));
		core_assert_always(_convolutionShader.setCoefficients(coeff));
		core_assert_always(_vbo.bind());
		blur(glowTexture, _bloom[0], false);
		blur(_bloom[0].texture(), _bloom[1], true);
	}

	// prepare the first source buffer by rendering the glow texture into it.
	{
		video::ScopedShader scoped(_textureShader);
		core_assert_always(_textureShader.setTexture(video::TextureUnit::Zero));
		_frameBuffers0[0].bind(true);
		video::bindTexture(video::TextureUnit::Zero, _bloom[1].texture());
		video::drawArrays(video::Primitive::Triangles, 6);
	}

	for (int i = 1; i < passes(); ++i) {
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
			video::drawArrays(video::Primitive::Triangles, 6);
		}
	}

	apply(_frameBuffers0, _frameBuffers1);

	{
		video::ScopedShader scoped(_combine2Shader);
		_bloom[0].bind(true);
		video::bindTexture(video::TextureUnit::Zero, glowTexture);
		video::bindTexture(video::TextureUnit::One, _frameBuffers1[0].texture());
		video::drawArrays(video::Primitive::Triangles, 6);
	}

	{
		if (!_yFlipped) {
			_vbo.unbind();
			_vbo.createFullscreenTextureBuffer(_texBufferIndex);
			_vbo.bind();
		}
		// restore previous fbo and viewport
		video::bindFramebuffer(oldFB);
		video::viewport(viewport[0], viewport[1], viewport[2], viewport[3]);
		video::ScopedShader scoped(_combine2Shader);
		video::bindTexture(video::TextureUnit::Zero, srcTexture);
		video::bindTexture(video::TextureUnit::One, _bloom[0].texture());
		video::drawArrays(video::Primitive::Triangles, 6);
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
	_convolutionShader.shutdown();
	_textureShader.shutdown();
	_combine2Shader.shutdown();
	if (_black) {
		_black->shutdown();
		_black = video::TexturePtr();
	}
	_vbo.shutdown();
}

} // namespace render
