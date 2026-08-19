/**
 * @file
 * @brief Deferred GPU resource destruction for multi-frame-in-flight backends.
 */

#pragma once

#include "Types.h"

namespace video {

enum class DestroyResourceType : uint8_t {
	Buffer,
	Texture,
	Framebuffer,
	Renderbuffer,
	VertexArray,
	Program,
	Shader,
	Max
};

void deferredDestroyAdvanceFrame();
void deferredDestroyEnqueue(DestroyResourceType type, Id id);
void deferredDestroyFlushAll();
size_t deferredDestroyPendingCount();
bool deferredDestroyEnabled();

} // namespace video
