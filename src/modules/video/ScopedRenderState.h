/**
 * @file
 */

#pragma once

#include "video/Types.h"
#include "math/Vec4i.h"

namespace video {

/**
 * @brief Ensure that the scoped where this object is created is properly reset to the state that was active at the
 * beginning of the scope
 * @note Little debug helper to find renderer state issues
 */
class ScopedRenderState {
private:
	struct State {
		math::Vec4i _scissor;
		math::Vec4i _viewport;
		Id _frameBuffer;
		bool _blendEnabled;
		BlendMode _blendSrc;
		BlendMode _blendDest;
		BlendEquation _blendFunc;
		CompareFunc _depthFunc;
		Id _vertexArray;
		Id _textures[core::enumVal(TextureUnit::Max)];

		void capture();
		void compare(const State &other);
	};
	State _state;

public:
	ScopedRenderState();

	~ScopedRenderState();
};

} // namespace video
