/**
 * @file
 */

#include "RendererState.h"

namespace video {

void RendererState::startFrame() {
	drawCalls = 0;

	// Clear pending uniforms as they're program-specific and may not be valid for the new frame
	pendingUniformi.clear();
}

} // namespace video
