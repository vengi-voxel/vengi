/**
 * @file
 */

#include "ScopedRenderState.h"
#include "core/Enum.h"
#include "video/Renderer.h"
#include "video/ScopedRenderBuffer.h"

namespace video {

void ScopedRenderState::State::capture() {
	getScissor(_scissor.x, _scissor.y, _scissor.z, _scissor.w);
	getViewport(_viewport.x, _viewport.y, _viewport.z, _viewport.w);
	_frameBuffer = currentFramebuffer();
	getBlendState(_blendEnabled, _blendSrc, _blendDest, _blendFunc);
	_depthFunc = getDepthFunc();
	_vertexArray = boundVertexArray();
	for (int i = 0; i < core::enumVal(TextureUnit::Max); ++i) {
		_textures[i] = currentTexture((TextureUnit)i);
	}
}

void ScopedRenderState::State::compare(const State &other) {
	core_assert(other._frameBuffer == _frameBuffer);
	core_assert(other._blendEnabled == _blendEnabled);
	core_assert(other._blendSrc == _blendSrc);
	core_assert(other._blendDest == _blendDest);
	core_assert(other._blendFunc == _blendFunc);
	core_assert(other._vertexArray == _vertexArray);
	core_assert(other._depthFunc == _depthFunc);
	core_assert(other._scissor == _scissor);
	core_assert(other._viewport == _viewport);
	for (int i = 0; i < core::enumVal(TextureUnit::Max); ++i) {
		core_assert_msg(other._textures[i] == _textures[i], "texture unit %i", i);
	}
}

ScopedRenderState::ScopedRenderState() {
	_state.capture();
}

ScopedRenderState::~ScopedRenderState() {
	State state;
	state.capture();
	state.compare(_state);
}

} // namespace video
