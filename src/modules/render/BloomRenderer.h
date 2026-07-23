/**
 * @file
 */

#pragma once

#include "ConvolutionData.h"
#include "ConvolutionShader.h"
#include "Combine2Shader.h"
#include "TextureShader.h"
#include "core/Var.h"
#include "video/Buffer.h"
#include "video/FrameBuffer.h"
#include "video/Texture.h"
#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

namespace render {

#define DOWNSAMPLE_PASSES 5

/**
 * @brief Renders a textures with the shader::BlurShader
 */
class BloomRenderer : public core::NonCopyable {
private:
	alignas(16) shader::ConvolutionData::ConvData _convolutionFragData;
	shader::ConvolutionData _convolutionData;
	shader::ConvolutionShader &_convolutionShader;
	shader::TextureShader &_textureShader;
	shader::Combine2Shader &_combine2Shader;
	video::Buffer _vbo;
	int _bufferIndex = -1;
	int _texBufferIndex = -1;
	bool _yFlipped = false;
	bool _uvBufferReady = false;
	int _lastBlurWidth = -1;
	int _lastBlurHorizontal = -1;
	video::FrameBuffer _bloom[2];
	video::FrameBuffer _frameBuffers0[DOWNSAMPLE_PASSES];
	video::FrameBuffer _frameBuffers1[DOWNSAMPLE_PASSES];
	video::FrameBuffer _frameBuffers2[DOWNSAMPLE_PASSES];
	video::TexturePtr _black;
	core::VarPtr _bloomPasses;

	void ensureUvBuffer();
	int activePasses() const;
	void apply(video::FrameBuffer *sources, video::FrameBuffer *dests, int passCount);
	void blur(const video::TexturePtr &source, video::FrameBuffer &dest, bool horizontal);

public:
	BloomRenderer();
	/**
	 * @sa shutdown()
	 */
	bool init(bool yFlipped, int width, int height);

	bool resize(int width, int height);

	static constexpr int passes() { return DOWNSAMPLE_PASSES; }

	/**
	 * @sa init()
	 */
	void shutdown();

	/**
	 * @param srcTexture The video::TexturePtr of the original texture
	 * @param glowTexture The video::TexturePtr of the glow texture that should get blurred and combined with
	 * the original texture
	 * @note The result is put into a texture that can get queried to continue to use it
	 * @see texture()
	 */
	void render(const video::TexturePtr& srcTexture, const video::TexturePtr& glowTexture);
	/**
	 * @return video::TexturePtr of the render() pass
	 */
	video::TexturePtr texture() const;
	video::TexturePtr texture0(int pass) const;
	video::TexturePtr texture1(int pass) const;
	video::TexturePtr texture2(int pass) const;
};

} // namespace render
