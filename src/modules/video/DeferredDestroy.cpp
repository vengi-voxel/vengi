/**
 * @file
 */

#include "DeferredDestroy.h"
#include "RenderStats.h"
#include "RendererInterface.h"
#include "core/collection/DynamicArray.h"

namespace video {

namespace {

struct PendingDestroy {
	DestroyResourceType type = DestroyResourceType::Max;
	Id id = InvalidId;
	uint64_t frameNumber = 0u;
};

static constexpr uint64_t FRAMES_IN_FLIGHT = 3u;

core::DynamicArray<PendingDestroy, 64> s_pending;
uint64_t s_frameNumber = 0u;
bool s_immediateMode = false;

void executeDestroy(DestroyResourceType type, Id id) {
	if (id == InvalidId) {
		return;
	}
	switch (type) {
	case DestroyResourceType::Buffer:
		deleteBuffers(1, &id);
		break;
	case DestroyResourceType::Texture:
		deleteTextures(1, &id);
		break;
	case DestroyResourceType::Framebuffer:
		deleteFramebuffers(1, &id);
		break;
	case DestroyResourceType::Renderbuffer:
		deleteRenderbuffers(1, &id);
		break;
	case DestroyResourceType::VertexArray:
		deleteVertexArrays(1, &id);
		break;
	case DestroyResourceType::Program: {
		Id copy = id;
		deleteProgram(copy);
		break;
	}
	case DestroyResourceType::Shader: {
		Id copy = id;
		deleteShader(copy);
		break;
	}
	default:
		break;
	}
}

void flushReadyDestroys() {
	if (s_pending.empty()) {
		return;
	}
	size_t write = 0u;
	for (size_t i = 0u; i < s_pending.size(); ++i) {
		const PendingDestroy &entry = s_pending[i];
		if (s_frameNumber < entry.frameNumber + FRAMES_IN_FLIGHT) {
			s_pending[write++] = entry;
			continue;
		}
		executeDestroy(entry.type, entry.id);
	}
	s_pending.resize(write);
}

} // namespace

bool deferredDestroyEnabled() {
	return !s_immediateMode;
}

void deferredDestroyAdvanceFrame() {
	++s_frameNumber;
	flushReadyDestroys();
}

void deferredDestroyEnqueue(DestroyResourceType type, Id id) {
	if (id == InvalidId) {
		return;
	}
	if (s_immediateMode) {
		executeDestroy(type, id);
		return;
	}
	PendingDestroy entry;
	entry.type = type;
	entry.id = id;
	entry.frameNumber = s_frameNumber;
	s_pending.push_back(entry);
	statsDeferredDestroy();
}

void deferredDestroyFlushAll() {
	waitDeviceIdle();
	s_immediateMode = true;
	for (const PendingDestroy &entry : s_pending) {
		executeDestroy(entry.type, entry.id);
	}
	s_pending.clear();
	s_immediateMode = false;
}

size_t deferredDestroyPendingCount() {
	return s_pending.size();
}

} // namespace video
