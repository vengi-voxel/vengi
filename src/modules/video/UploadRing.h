/**
 * @file
 * @brief Persistent per-frame upload staging for buffer data
 */

#pragma once

#include "Types.h"

namespace video {

struct UploadRingAllocation {
	void *cpuPtr = nullptr;
	size_t offset = 0u;
	Id ringBuffer = InvalidId;
	bool valid = false;
};

bool uploadRingEnabled();
bool uploadRingInit();
void uploadRingShutdown();
void uploadRingBeginFrame();

void uploadRingEndFrame();

UploadRingAllocation uploadRingAlloc(BufferType type, size_t size);
bool uploadRingCopyToBuffer(const UploadRingAllocation &alloc, Id dest, BufferType destType, intptr_t destOffset,
							size_t size);

} // namespace video
