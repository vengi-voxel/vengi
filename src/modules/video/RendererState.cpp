/**
 * @file
 */

#include "RendererState.h"

namespace video {

void RendererState::startFrame() {
	drawCalls = 0;

	// Synchronize pending states with their current values to prevent
	// superfluous state changes from a previous frame
	for (int i = 0; i < core::enumVal(TextureUnit::Max); ++i) {
		pendingTextureHandle[i] = textureHandle[i];
		// pendingTextureType[i] = TextureType::Max;
	}

	pendingClearColor = clearColor;
	pendingCullFace = cullFace;
	pendingDepthFunc = depthFunc;
	pendingPolygonOffset = polygonOffset;
	pendingPointSize = pointSize;
	pendingPolygonModeFace = polygonModeFace;
	pendingPolygonMode = polygonMode;
	pendingBlendSrcRGB = blendSrcRGB;
	pendingBlendDestRGB = blendDestRGB;
	pendingBlendSrcAlpha = blendSrcAlpha;
	pendingBlendDestAlpha = blendDestAlpha;
	pendingBlendEquation = blendEquation;
	pendingViewportX = viewportX;
	pendingViewportY = viewportY;
	pendingViewportW = viewportW;
	pendingViewportH = viewportH;
	pendingScissorX = scissorX;
	pendingScissorY = scissorY;
	pendingScissorW = scissorW;
	pendingScissorH = scissorH;
	pendingStates = states;
	for (int i = 0; i < 4; ++i) {
		pendingColorMask[i] = colorMask[i];
	}
	pendingLineWidth = lineWidth;
	pendingProgramHandle = programHandle;
	// Clear pending uniforms as they're program-specific and may not be valid for the new frame
	pendingUniformi.clear();
}

} // namespace video
