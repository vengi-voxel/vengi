/**
 * @file
 */

#include "Renderer.h"
#include "core/Log.h"
#include "core/Assert.h"
#include <SDL_video.h>

namespace video {

void deleteRenderbuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteRenderbuffers(1, &id);
	id = InvalidId;
}

void deleteFramebuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteFramebuffers(1, &id);
	id = InvalidId;
}

void deleteTexture(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteTextures(1, &id);
	id = InvalidId;
}

Id genTexture() {
	Id id;
	genTextures(1, &id);
	return id;
}

void deleteBuffer(Id& id) {
	if (id == InvalidId) {
		return;
	}
	deleteBuffers(1, &id);
	id = InvalidId;
}

Id genVertexArray() {
	Id id;
	genVertexArrays(1, &id);
	return id;
}

Id genBuffer() {
	Id id;
	genBuffers(1, &id);
	return id;
}

Id genRenderbuffer() {
	Id id;
	genRenderbuffers(1, &id);
	return id;
}

Id genFramebuffer() {
	Id id;
	genFramebuffers(1, &id);
	return id;
}

void disableDebug() {
	if (!hasFeature(Feature::DebugOutput)) {
		return;
	}
	disable(State::DebugOutput);
	Log::info("disable render debug messages");
}

bool checkLimit(int amount, Limit l) {
	const int v = renderState().limit(l);
	if (v <= 0) {
		Log::trace("No limit found for %i", (int)l);
		return true;
	}
	return v >= amount;
}

}
