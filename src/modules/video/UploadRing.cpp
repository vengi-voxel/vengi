/**
 * @file
 */

#include "UploadRing.h"
#include "Renderer.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/sdl/SDLSystem.h"

namespace video {

namespace {

static constexpr int FRAMES_IN_FLIGHT = 3;
static constexpr size_t RING_SIZE_VERTEX = 8u * 1024u * 1024u;
static constexpr size_t RING_SIZE_INDEX = 4u * 1024u * 1024u;
static constexpr size_t RING_SIZE_UNIFORM = 1u * 1024u * 1024u;

enum class RingKind : uint8_t { Vertex = 0, Index = 1, Uniform = 2, Max };

struct RingSlot {
	Id handle = InvalidId;
	void *mapped = nullptr;
	size_t capacity = 0u;
	size_t offset = 0u;
};

RingSlot s_slots[FRAMES_IN_FLIGHT][core::enumVal(RingKind::Max)];
Fence s_fences[FRAMES_IN_FLIGHT]{};
int s_currentSlot = 0;
bool s_initialized = false;

RingKind ringKindFor(BufferType type) {
	switch (type) {
	case BufferType::ArrayBuffer:
		return RingKind::Vertex;
	case BufferType::IndexBuffer:
		return RingKind::Index;
	case BufferType::UniformBuffer:
		return RingKind::Uniform;
	default:
		return RingKind::Max;
	}
}

size_t ringCapacity(RingKind kind) {
	switch (kind) {
	case RingKind::Vertex:
		return RING_SIZE_VERTEX;
	case RingKind::Index:
		return RING_SIZE_INDEX;
	case RingKind::Uniform:
		return RING_SIZE_UNIFORM;
	default:
		return 0u;
	}
}

BufferType ringBufferType(RingKind kind) {
	switch (kind) {
	case RingKind::Vertex:
		return BufferType::ArrayBuffer;
	case RingKind::Index:
		return BufferType::IndexBuffer;
	case RingKind::Uniform:
		return BufferType::UniformBuffer;
	default:
		return BufferType::Max;
	}
}

size_t ringAlignment(BufferType type) {
	size_t alignment = 32u;
	if (type == BufferType::IndexBuffer) {
		alignment = 16u;
	} else if (type == BufferType::UniformBuffer) {
		const int uboAlign = specificationi(Spec::UniformBufferAlignment);
		if (uboAlign > 0) {
			alignment = (size_t)uboAlign;
		}
	}
	return alignment;
}

size_t alignUp(size_t value, size_t alignment) {
	if (alignment <= 1u) {
		return value;
	}
	return (value + alignment - 1u) & ~(alignment - 1u);
}

void resetRingSlot(RingSlot &slot) {
	slot.handle = InvalidId;
	slot.mapped = nullptr;
	slot.capacity = 0u;
	slot.offset = 0u;
}

void destroyRingSlot(RingSlot &slot, BufferType type) {
	if (slot.handle != InvalidId) {
		if (slot.mapped != nullptr) {
			unmapBuffer(slot.handle, type);
			slot.mapped = nullptr;
		}
		Id ids[1] = {slot.handle};
		deleteBuffers(1, ids);
	}
	resetRingSlot(slot);
}

bool createRingBuffer(RingSlot &slot, BufferType type, size_t capacity) {
	if (capacity == 0u || !hasFeature(Feature::BufferStorage)) {
		return false;
	}
	slot.handle = genBuffer();
	if (slot.handle == InvalidId) {
		return false;
	}
	slot.capacity = capacity;
	slot.mapped = nullptr;

	const BufferStorageFlag persistentStorage = BufferStorageFlag::Dynamic | BufferStorageFlag::MapWrite |
												BufferStorageFlag::MapPersistent | BufferStorageFlag::MapCoherent;
	if (bufferStorage(slot.handle, type, capacity, persistentStorage)) {
		slot.mapped = mapBufferRange(slot.handle, type, 0, capacity, AccessMode::Write,
									 MapBufferFlag::Persistent | MapBufferFlag::Coherent);
		if (slot.mapped != nullptr) {
			return true;
		}
	}
	destroyRingSlot(slot, type);
	// Non-persistent mapped buffers cannot be copy sources until unmapped.
	// Use ordinary direct uploads when persistent coherent mapping is unavailable.
	return false;
}

} // namespace

bool uploadRingEnabled() {
	return s_initialized;
}

bool uploadRingInit() {
	if (s_initialized) {
		return true;
	}
	if (!hasFeature(Feature::BufferStorage)) {
		Log::info("Upload ring disabled: no buffer storage support");
		return false;
	}
	for (int slot = 0; slot < FRAMES_IN_FLIGHT; ++slot) {
		deleteFence(s_fences[slot]);
		for (int kind = 0; kind < core::enumVal(RingKind::Max); ++kind) {
			const RingKind ringKind = (RingKind)kind;
			const BufferType bufferType = ringBufferType(ringKind);
			const size_t capacity = ringCapacity(ringKind);
			if (!createRingBuffer(s_slots[slot][kind], bufferType, capacity)) {
				uploadRingShutdown();
				Log::warn("Failed to initialize upload ring");
				return false;
			}
			s_slots[slot][kind].offset = 0u;
		}
	}
	s_currentSlot = 0;
	s_initialized = true;
	Log::info("Upload ring initialized (%i in-flight slots)", FRAMES_IN_FLIGHT);
	return true;
}

void uploadRingShutdown() {
	for (int slot = 0; slot < FRAMES_IN_FLIGHT; ++slot) {
		deleteFence(s_fences[slot]);
		for (int kind = 0; kind < core::enumVal(RingKind::Max); ++kind) {
			destroyRingSlot(s_slots[slot][kind], ringBufferType((RingKind)kind));
		}
	}
	s_initialized = false;
	s_currentSlot = 0;
}

static void sealSlot(int index) {
	deleteFence(s_fences[index]);
	s_fences[index] = createFence();
	if (!s_fences[index])
		waitDeviceIdle();
}

void uploadRingBeginFrame() {
	if (!s_initialized)
		return;
	// Initialization may already have staged uploads before the first frame.
	if (!s_fences[s_currentSlot]) {
		for (const auto &slot : s_slots[s_currentSlot]) {
			if (slot.offset > 0) {
				sealSlot(s_currentSlot);
				break;
			}
		}
	}
	s_currentSlot = (s_currentSlot + 1) % FRAMES_IN_FLIGHT;
	// Coherent mapping provides visibility, not ownership. Never overwrite bytes
	// still consumed by the GPU; yield while waiting rather than busy-spinning.
	while (!fenceSignaled(s_fences[s_currentSlot]))
		SDL_Delay(1);
	deleteFence(s_fences[s_currentSlot]);
	for (auto &slot : s_slots[s_currentSlot])
		slot.offset = 0u;
}

void uploadRingEndFrame() {
	if (!s_initialized)
		return;
	for (const auto &slot : s_slots[s_currentSlot]) {
		if (slot.offset > 0) {
			sealSlot(s_currentSlot);
			break;
		}
	}
}

UploadRingAllocation uploadRingAlloc(BufferType type, size_t size) {
	UploadRingAllocation result;
	if (!s_initialized || size == 0u) {
		return result;
	}
	const RingKind kind = ringKindFor(type);
	if (kind == RingKind::Max) {
		return result;
	}
	RingSlot &slot = s_slots[s_currentSlot][core::enumVal(kind)];
	const size_t alignment = ringAlignment(type);
	const size_t alignedOffset = alignUp(slot.offset, alignment);
	if (alignedOffset + size > slot.capacity || slot.mapped == nullptr) {
		return result;
	}
	result.cpuPtr = (uint8_t *)slot.mapped + alignedOffset;
	result.offset = alignedOffset;
	result.ringBuffer = slot.handle;
	result.valid = true;
	slot.offset = alignedOffset + size;
	return result;
}

bool uploadRingCopyToBuffer(const UploadRingAllocation &alloc, Id dest, BufferType destType, intptr_t destOffset,
							size_t size) {
	(void)destType;
	if (!alloc.valid || dest == InvalidId || size == 0u) {
		return false;
	}
	return copyBufferSubData(alloc.ringBuffer, (intptr_t)alloc.offset, dest, destOffset, size);
}

} // namespace video
