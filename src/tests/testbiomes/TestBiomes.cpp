/**
 * @file
 */

#include "TestBiomes.h"
#include "testcore/TestAppMain.h"
#include "video/ScopedViewPort.h"
#include "video/Texture.h"
#include "core/ThreadPool.h"

TestBiomes::TestBiomes(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testbiomes");
}

void TestBiomes::recalcBiomes(const glm::ivec3& pos) {
	uint8_t *humidity = new uint8_t[_biomesTextureSize.x * _biomesTextureSize.y * 4];
	uint8_t *h = humidity;
	for (int y = 0; y < _biomesTextureSize.y; ++y) {
		for (int x = 0; x < _biomesTextureSize.x; ++x) {
			const voxelworld::Biome * b = _biomeMgr.getBiome(pos);
			*h++ = (int8_t)(b->humidity * 255.0f);
			*h++ = (int8_t)(b->temperature * 255.0f);
			*h++ = 0;
			*h++ = 255;
		}
	}
	_resultQueue.push(new BiomesTextureResult(humidity, _biomesTextureSize));
}

core::AppState TestBiomes::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	_logLevelVar->setVal(core::string::toString(SDL_LOG_PRIORITY_DEBUG));
	Log::init();
	video::clearColor(::core::Color::Black);

	if (!_renderer.init(frameBufferDimension())) {
		Log::error("Failed to setup the renderer");
		return core::AppState::InitFailure;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the material colors");
		return core::AppState::InitFailure;
	}

	const core::String& biomesData = io::filesystem()->load("biomes.lua");
	if (!_biomeMgr.init(biomesData)) {
		return core::AppState::InitFailure;
	}

	video::TextureConfig cfg2d;
	cfg2d.type(video::TextureType::Texture2D).format(video::TextureFormat::RGBA);
	_texture = video::createTexture(cfg2d, _biomesTextureSize.x, _biomesTextureSize.y);

	threadPool().enqueue([this] () {
		for (;;) {
			Event* event;
			if (!this->_workQueue.waitAndPop(event)) {
				break;
			}
			switch (event->type) {
			case EventType::RecalcBiomesForPosition:
				RecalcEvent* rce = (RecalcEvent*)event;
				recalcBiomes(rce->pos);
				delete rce;
				break;
			}
		}
	});

	// TODO: render humidity, temperature and mapped biomes
	// TODO: clicking on a position should print the biome that was chosen
	// TODO: put heightmap below the clicking area to simulate the lower and upper bounds for biome selection

	return state;
}

core::AppState TestBiomes::onRunning() {
	core::AppState state = Super::onRunning();
	Result* result = nullptr;
	if (_resultQueue.pop(result)) {
		switch (result->type) {
		case ResultType::BiomesTexture:
			BiomesTextureResult* r = (BiomesTextureResult*)result;
			_texture->upload(r->size.x, r->size.y, r->data);
			delete[] r->data;
			delete r;
			break;
		}
	}
	return state;
}

core::AppState TestBiomes::onCleanup() {
	core::AppState state = Super::onCleanup();
	_biomeMgr.shutdown();
	_workQueue.abortWait();
	// TODO: clear mem: while () {}

	if (_texture) {
		_texture->shutdown();
	}
	_renderer.shutdown();
	return state;
}

void TestBiomes::doRender() {
	video::ScopedTexture texture(_texture, video::TextureUnit::Zero);
	video::ScopedViewPort viewPort(0, 0, frameBufferDimension().x, frameBufferDimension().y);
	_renderer.render(_camera.projectionMatrix());
}

void TestBiomes::onRenderUI() {
	static bool biomesOpened = true;
	if (ImGui::Begin("Biomes", &biomesOpened)) {
		if (ImGui::InputVec3("pos", _biomesPos)) {
			_workQueue.push(new RecalcEvent(_biomesPos));
		}
		// TODO: render biome image to imgui widget
	}
	ImGui::End();

	static bool heightmapOpened = true;
	if (ImGui::Begin("Heightmap", &heightmapOpened)) {
		// TODO: tweak heightmap noise parameters and render to imgui widget
	}
	ImGui::End();

	ImGui::Separator();
	if (ImGui::Button("Quit")) {
		requestQuit();
	}
}

TEST_APP(TestBiomes)
