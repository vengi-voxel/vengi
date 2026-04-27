/**
 * @file
 */

#include "../RenderPanel.h"
#include "../WindowTitles.h"
#include "voxedit-util/SceneManager.h"
#include "voxelpathtracer/PathTracerState.h"

namespace voxedit {

void RenderPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "settings inputs")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, TITLE_RENDERSETTINGS));
		voxelpathtracer::PathTracerState &state = _pathTracer.state();
		yocto::trace_params &params = state.params;

		const int oldResolution = params.resolution;
		ctx->ItemInputValue("Dimensions", oldResolution + 128);
		IM_CHECK(params.resolution == oldResolution + 128);

		const int oldSamples = params.samples;
		ctx->ItemInputValue("Samples", oldSamples + 16);
		IM_CHECK(params.samples == oldSamples + 16);
	};

	IM_REGISTER_TEST(engine, testCategory(), "presets")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(focusWindow(ctx, TITLE_RENDERSETTINGS));
		voxelpathtracer::PathTracerState &state = _pathTracer.state();
		yocto::trace_params &params = state.params;

		ctx->ItemClick("High quality");
		ctx->Yield();
		IM_CHECK_EQ(params.samples, 1024);
		IM_CHECK_EQ(params.bounces, 64);

		ctx->ItemClick("Geometry preview");
		ctx->Yield();
		IM_CHECK_EQ(params.samples, 16);

		ctx->ItemClick("Reset all");
		ctx->Yield();
		yocto::trace_params defaults;
		IM_CHECK_EQ(params.samples, defaults.samples);
		IM_CHECK_EQ(params.bounces, defaults.bounces);
	};
}

} // namespace voxedit
