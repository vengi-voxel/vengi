/**
 * @file
 */

#include "RenderStats.h"
#include "core/Log.h"
#include "core/TimeProvider.h"
#include <inttypes.h>

namespace video {

namespace {

RenderStats s_frame;
RenderStats s_totals;
uint64_t s_frameStartTicks = 0u;
uint64_t s_uploadScopeStartTicks = 0u;
uint64_t s_uploadScopeDepth = 0u;
bool s_frameOpen = false;
double s_displayCpuRenderMs = 0.0;
double s_displayCpuUploadMs = 0.0;
double s_uploadThisFrameMs = 0.0;

inline double ticksToMs(uint64_t ticks) {
	const uint64_t freq = core::TimeProvider::highResTimeResolution();
	if (freq == 0u) {
		return 0.0;
	}
	return ticks * 1000.0 / (double)freq;
}

void accumulateTotals(const RenderStats &frame) {
	s_totals.drawCalls += frame.drawCalls;
	s_totals.pipelineBinds += frame.pipelineBinds;
	s_totals.descriptorBinds += frame.descriptorBinds;
	s_totals.bufferUpdates += frame.bufferUpdates;
	s_totals.bufferBytesUploaded += frame.bufferBytesUploaded;
	s_totals.resourceCreates += frame.resourceCreates;
	s_totals.resourceDestroys += frame.resourceDestroys;
	s_totals.renderPasses += frame.renderPasses;
	s_totals.fullscreenPasses += frame.fullscreenPasses;
	s_totals.blits += frame.blits;
	s_totals.cpuRenderMs += frame.cpuRenderMs;
	s_totals.cpuUploadMs += frame.cpuUploadMs;
	s_totals.stateChangeSkipped += frame.stateChangeSkipped;
	s_totals.deferredDestroyCount += frame.deferredDestroyCount;
	s_totals.frameNumber = frame.frameNumber;
}

} // namespace

const RenderStats &renderStats() {
	return s_frame;
}

const RenderStats &renderStatsTotals() {
	return s_totals;
}

void beginFrameStats() {
	s_frame = RenderStats {};
	s_frame.frameNumber = s_totals.frameNumber + 1u;
	s_frame.cpuRenderMs = s_displayCpuRenderMs;
	s_frame.cpuUploadMs = s_displayCpuUploadMs;
	s_frameStartTicks = core::TimeProvider::highResTime();
	s_frameOpen = true;
	s_uploadThisFrameMs = 0.0;
}

void endFrameStats() {
	if (!s_frameOpen) {
		return;
	}
	s_frameOpen = false;
	if (s_frameStartTicks != 0u) {
		const uint64_t now = core::TimeProvider::highResTime();
		s_frame.cpuRenderMs = ticksToMs(now - s_frameStartTicks);
	}
	s_frame.cpuUploadMs = s_uploadThisFrameMs;
	s_displayCpuRenderMs = s_frame.cpuRenderMs;
	s_displayCpuUploadMs = s_frame.cpuUploadMs;
	accumulateTotals(s_frame);
	Log::debug(
		"render-stats {\"frame\":%" PRIu64
		",\"draw_calls\":%" PRIu64 ",\"pipeline_binds\":%" PRIu64 ",\"descriptor_binds\":%" PRIu64
		",\"buffer_updates\":%" PRIu64 ",\"buffer_bytes_uploaded\":%" PRIu64
		",\"resource_creates\":%" PRIu64 ",\"resource_destroys\":%" PRIu64
		",\"render_passes\":%" PRIu64 ",\"fullscreen_passes\":%" PRIu64 ",\"blits\":%" PRIu64
		",\"cpu_render_ms\":%.3f,\"cpu_upload_ms\":%.3f,\"state_change_skipped\":%" PRIu64
		",\"deferred_destroy_count\":%" PRIu64 "}",
		s_frame.frameNumber, s_frame.drawCalls, s_frame.pipelineBinds, s_frame.descriptorBinds,
		s_frame.bufferUpdates, s_frame.bufferBytesUploaded, s_frame.resourceCreates, s_frame.resourceDestroys,
		s_frame.renderPasses, s_frame.fullscreenPasses, s_frame.blits, s_frame.cpuRenderMs, s_frame.cpuUploadMs,
		s_frame.stateChangeSkipped, s_frame.deferredDestroyCount);
}

void logRenderStatsTotals() {
	Log::info(
		"render-stats totals over %" PRIu64
		" frames: draw_calls=%" PRIu64 " pipeline_binds=%" PRIu64 " descriptor_binds=%" PRIu64
		" buffer_updates=%" PRIu64 " buffer_bytes=%" PRIu64 " resource_creates=%" PRIu64
		" resource_destroys=%" PRIu64 " render_passes=%" PRIu64 " fullscreen_passes=%" PRIu64
		" blits=%" PRIu64 " cpu_render_ms=%.3f cpu_upload_ms=%.3f state_change_skipped=%" PRIu64
		" deferred_destroy_count=%" PRIu64,
		s_totals.frameNumber, s_totals.drawCalls, s_totals.pipelineBinds, s_totals.descriptorBinds,
		s_totals.bufferUpdates, s_totals.bufferBytesUploaded, s_totals.resourceCreates, s_totals.resourceDestroys,
		s_totals.renderPasses, s_totals.fullscreenPasses, s_totals.blits, s_totals.cpuRenderMs, s_totals.cpuUploadMs,
		s_totals.stateChangeSkipped, s_totals.deferredDestroyCount);
}

void statsDrawCall() {
	++s_frame.drawCalls;
}

void statsDrawCall(uint32_t count) {
	s_frame.drawCalls += count;
}

void statsPipelineBind() {
	++s_frame.pipelineBinds;
}

void statsDescriptorBind(uint32_t count) {
	s_frame.descriptorBinds += count;
}

void statsBufferUpdate(size_t bytes) {
	++s_frame.bufferUpdates;
	s_frame.bufferBytesUploaded += (uint64_t)bytes;
}

void statsResourceCreate(uint32_t count) {
	s_frame.resourceCreates += count;
}

void statsResourceDestroy(uint32_t count) {
	s_frame.resourceDestroys += count;
}

void statsRenderPass() {
	++s_frame.renderPasses;
}

void statsFullscreenPass() {
	++s_frame.fullscreenPasses;
}

void statsBlit() {
	++s_frame.blits;
}

void statsStateChangeSkipped() {
	++s_frame.stateChangeSkipped;
}

void statsDeferredDestroy(uint32_t count) {
	s_frame.deferredDestroyCount += count;
}

void statsUploadScopeBegin() {
	if (s_uploadScopeDepth++ == 0u) {
		s_uploadScopeStartTicks = core::TimeProvider::highResTime();
	}
}

void statsUploadScopeEnd() {
	if (s_uploadScopeDepth == 0u) {
		return;
	}
	--s_uploadScopeDepth;
	if (s_uploadScopeDepth == 0u && s_uploadScopeStartTicks != 0u) {
		const uint64_t now = core::TimeProvider::highResTime();
		const double dt = ticksToMs(now - s_uploadScopeStartTicks);
		s_uploadThisFrameMs += dt;
		s_frame.cpuUploadMs = s_displayCpuUploadMs + s_uploadThisFrameMs;
		s_uploadScopeStartTicks = 0u;
	}
}

} // namespace video
