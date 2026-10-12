/**
 * @file
 */

#include "app/benchmark/AbstractBenchmark.h"
#include "core/ConfigVar.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "core/sdl/SDLSystem.h"
#include "engine-config.h"
#include "image/Image.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Renderer.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelpathtracer/PathTracer.h"

class PathTracerBenchmark : public app::AbstractBenchmark {
protected:
	SDL_Window *_window = nullptr;
	video::RendererContext _context = nullptr;
	voxelpathtracer::PathTracer _pathTracer;
	bool _ready = false;
	video::Id _timerQuery = video::InvalidId;
	static constexpr int Samples = 16;

	bool onInitApp() override {
		core::Var::registerVar(core::VarDef(cfg::ClientOpenGLVersion, "4.3", "", "", core::CV_READONLY));
		core::Var::registerVar(core::VarDef(cfg::ClientMultiSampleBuffers, 0, "", ""));
		core::Var::registerVar(core::VarDef(cfg::ClientMultiSampleSamples, 0, "", ""));
		core::Var::registerVar(core::VarDef(cfg::ClientVSync, false, "", ""));
		core::Var::registerVar(core::VarDef(cfg::RenderMaxAnisotropy, 1.0f, "", ""));
		core::Var::registerVar(core::VarDef(cfg::ClientDebugSeverity, 3, "", ""));
		if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
			return false;
		}
		video::setup();
		video::construct();
		SDL_WindowFlags flags = SDL_WINDOW_HIDDEN;
#if USE_GL_RENDERER
		flags |= SDL_WINDOW_OPENGL;
#elif USE_VK_RENDERER
		flags |= SDL_WINDOW_VULKAN;
#endif
		_window = SDL_CreateWindow("pathtracer benchmark", 128, 128, flags);
		if (_window == nullptr) {
			return false;
		}
		_context = video::createContext(_window);
		_ready = _context != nullptr && video::init(128, 128, 1.0f);
		if (_ready) {
			_timerQuery = video::createTimerQuery();
		}
		return _ready;
	}

	void onCleanupApp() override {
		_pathTracer.shutdown();
		video::deleteTimerQuery(_timerQuery);
		if (_context != nullptr) {
			video::destroyContext(_context);
		}
		if (_window != nullptr) {
			SDL_DestroyWindow(_window);
			_window = nullptr;
		}
		_ready = false;
	}

	void renderBatch() {
		_pathTracer.resetAccumulation();
		while (!_pathTracer.update()) {
		}
	}

public:
	void SetUp(benchmark::State &state) override {
		app::AbstractBenchmark::SetUp(state);
		if (!_ready) {
			state.SkipWithError("Could not initialize the renderer benchmark context");
			return;
		}
		const int scenario = (int)state.range(0);
		const int size = scenario == 5 ? 512 : (scenario == 3 ? 128 : 64);
		scenegraph::SceneGraph sceneGraph;
		scenegraph::SceneGraphNode model(scenegraph::SceneGraphNodeType::Model);
		voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(0, 0, 0, size - 1, 63, size - 1));
		palette::Palette pal;
		pal.setColor(1, color::RGBA(180, 140, 80, 255));
		pal.setColor(2, color::RGBA(255, 220, 180, 255));
		pal.setMaterialType(2, palette::MaterialType::Emit);
		pal.setEmit(2, 1.0f);
		if (scenario == 2) {
			pal.setMaterialType(1, palette::MaterialType::Glass);
		}
		const int spacing = scenario == 5 ? 128 : (scenario == 3 ? 48 : 16);
		for (int z = 4; z + 7 < size; z += spacing) {
			for (int x = 4; x + 7 < size; x += spacing) {
				for (int dz = 0; dz < 8; ++dz) {
					for (int dy = 0; dy < 16; ++dy) {
						for (int dx = 0; dx < 8; ++dx) {
							volume->setVoxel(x + dx, 4 + dy, z + dz, voxel::createVoxel(pal, 1));
						}
					}
				}
			}
		}
		if (scenario == 4) {
			for (int z = 28; z < 36; ++z) {
				for (int x = 28; x < 36; ++x) {
					volume->setVoxel(x, 32, z, voxel::createVoxel(pal, 2));
				}
			}
		}
		if (scenario == 6) {
			delete volume;
		} else {
			model.setVolume(volume);
			model.setPalette(pal);
			sceneGraph.emplace(core::move(model));
		}
		sceneGraph.updateTransforms();
		voxelpathtracer::PathTracerParams &params = _pathTracer.state().params;
		params.resolution = (int)state.range(1);
		params.samples = Samples;
		params.samplesThisPass = (int)state.range(2);
		params.bounces = 4;
		params.voxelEdge = scenario == 1 ? 1.0f : 0.0f;
		params.sunIntensity = scenario == 4 ? 0.0f : 3.0f;
		video::Camera camera;
		camera.setSize(glm::ivec2(params.resolution));
		voxelrender::configureCamera(camera, voxel::Region(0, 0, 0, size - 1, 23, size - 1),
									 voxelrender::SceneCameraMode::Free, (float)size * 4.0f);
		camera.update(0.0);
		if (!_pathTracer.start(sceneGraph, &camera)) {
			state.SkipWithError("Could not initialize the path tracer scene");
			return;
		}
		const char *labels[] = {"opaque", "rounded", "glass", "sparse", "emissive", "large sparse", "empty"};
		state.SetLabel(core::String::format("%s; %s", labels[scenario],
											_timerQuery == video::InvalidId ? "synchronized wall time" : "GPU time")
						   .c_str());
		// Compile and warm shader/texture caches before starting measurement.
		renderBatch();
		renderBatch();
		video::finish();
	}
};

BENCHMARK_DEFINE_F(PathTracerBenchmark, Trace)(benchmark::State &state) {
	if (state.skipped()) {
		return;
	}
	const double frequency = (double)core::TimeProvider::highResTimeResolution();
	double elapsedSeconds = 0.0;
	for (auto _ : state) {
		const uint64_t start = core::TimeProvider::highResTime();
		if (_timerQuery != video::InvalidId) {
			video::beginTimerQuery(_timerQuery);
		}
		renderBatch();
		if (_timerQuery != video::InvalidId) {
			video::endTimerQuery();
			// Retrieving the result waits for the measured GPU commands to finish.
			state.SetIterationTime((double)video::timerQueryResult(_timerQuery) * 1e-9);
		} else {
			video::finish();
			state.SetIterationTime((double)(core::TimeProvider::highResTime() - start) / frequency);
		}
		elapsedSeconds += (double)(core::TimeProvider::highResTime() - start) / frequency;
	}
	state.counters["endToEnd_ms"] = elapsedSeconds * 1000.0 / (double)state.iterations();
	state.SetItemsProcessed(state.iterations() * state.range(1) * state.range(1) * Samples);
}

// Compare binding the GPU display texture with downloading the image each frame.
BENCHMARK_DEFINE_F(PathTracerBenchmark, Preview)(benchmark::State &state) {
	if (state.skipped()) {
		return;
	}
	const bool readback = state.range(3) != 0;
	video::TexturePtr upload = readback ? video::createEmptyTexture("preview-benchmark") : video::TexturePtr();
	const double frequency = (double)core::TimeProvider::highResTimeResolution();
	for (auto _ : state) {
		const uint64_t start = core::TimeProvider::highResTime();
		_pathTracer.resetAccumulation();
		bool done;
		do {
			done = _pathTracer.update();
			if (readback) {
				upload->upload(_pathTracer.image());
			} else {
				_pathTracer.texture();
			}
		} while (!done);
		video::finish();
		state.SetIterationTime((double)(core::TimeProvider::highResTime() - start) / frequency);
	}
	if (upload) {
		upload->shutdown();
	}
	state.SetLabel(readback ? "preview download/upload; synchronized wall time"
							: "GPU preview; synchronized wall time");
	state.SetItemsProcessed(state.iterations() * state.range(1) * state.range(1) * Samples);
}

BENCHMARK_REGISTER_F(PathTracerBenchmark, Preview)
	->ArgNames({"scene", "resolution", "samplesPerPass", "readback"})
	->Args({0, 512, 1, 0})
	->Args({0, 512, 1, 1})
	->UseManualTime()
	->Unit(benchmark::kMillisecond);

BENCHMARK_REGISTER_F(PathTracerBenchmark, Trace)
	->ArgNames({"scene", "resolution", "samplesPerPass"})
	->Args({0, 128, 8})
	->Args({1, 128, 8})
	->Args({2, 128, 8})
	->Args({3, 128, 8})
	->Args({4, 128, 8})
	->Args({5, 128, 8})
	->Args({0, 128, 1})
	->Args({0, 128, 16})
	->Args({0, 256, 8})
	->UseManualTime()
	->Unit(benchmark::kMillisecond);

// One editor frame, either displaying a finished image or computing one sample.
BENCHMARK_DEFINE_F(PathTracerBenchmark, InteractiveFrame)(benchmark::State &state) {
	if (state.skipped()) {
		return;
	}
	_pathTracer.state().params.bloom = state.range(3) != 0 ? 0.35f : 0.0f;
	_pathTracer.texture();
	video::finish();
	const bool active = state.range(4) != 0;
	for (auto _ : state) {
		if (_timerQuery != video::InvalidId) {
			video::beginTimerQuery(_timerQuery);
		}
		const uint64_t start = core::TimeProvider::highResTime();
		if (active) {
			_pathTracer.resetAccumulation();
			_pathTracer.update();
		}
		_pathTracer.texture();
		video::finish();
		const double seconds =
			(double)(core::TimeProvider::highResTime() - start) / (double)core::TimeProvider::highResTimeResolution();
		if (_timerQuery != video::InvalidId) {
			video::endTimerQuery();
			state.SetIterationTime((double)video::timerQueryResult(_timerQuery) * 1e-9);
		} else {
			state.SetIterationTime(seconds);
		}
		state.counters["endToEnd_ms"] += seconds * 1000.0;
	}
	state.counters["endToEnd_ms"] /= (double)state.iterations();
}

BENCHMARK_REGISTER_F(PathTracerBenchmark, InteractiveFrame)
	->ArgNames({"scene", "resolution", "samplesPerPass", "bloom", "active"})
	->Args({6, 512, 1, 0, 0})
	->Args({6, 512, 1, 1, 0})
	->Args({6, 512, 1, 0, 1})
	->Args({6, 512, 1, 1, 1})
	->Args({0, 512, 1, 0, 0})
	->Args({0, 512, 1, 1, 0})
	->Args({0, 512, 1, 0, 1})
	->Args({0, 512, 1, 1, 1})
	->Iterations(32)
	->UseManualTime()
	->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
