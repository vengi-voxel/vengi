/**
 * @file
 * @brief Per-frame renderer counters shared by all backends (GL, Vulkan, SDL_GPU).
 */

#pragma once

#include "core/Common.h"

namespace video {

/**
 * Lightweight counters for renderer CPU/GPU submission work.
 * Reset at the start of each frame; accumulated into totals at frame end.
 */
struct RenderStats {
	uint64_t drawCalls = 0;
	uint64_t pipelineBinds = 0;
	uint64_t descriptorBinds = 0;
	uint64_t bufferUpdates = 0;
	uint64_t bufferBytesUploaded = 0;
	uint64_t resourceCreates = 0;
	uint64_t resourceDestroys = 0;
	uint64_t renderPasses = 0;
	uint64_t fullscreenPasses = 0;
	uint64_t blits = 0;
	double cpuRenderMs = 0.0;
	double cpuUploadMs = 0.0;
	uint64_t frameNumber = 0;
	uint64_t stateChangeSkipped = 0;
};

const RenderStats &renderStats();
const RenderStats &renderStatsTotals();

void beginFrameStats();
void endFrameStats();
void logRenderStatsTotals();

void statsDrawCall();
void statsPipelineBind();
void statsDescriptorBind(uint32_t count = 1u);
void statsBufferUpdate(size_t bytes);
void statsResourceCreate(uint32_t count = 1u);
void statsResourceDestroy(uint32_t count = 1u);
void statsRenderPass();
void statsFullscreenPass();
void statsBlit();
void statsStateChangeSkipped();

void statsUploadScopeBegin();
void statsUploadScopeEnd();

} // namespace video
