/**
 * @file
 */

#include "TestBloom.h"
#include "image/Image.h"
#include "imgui.h"
#include "testcore/TestAppMain.h"
#include "video/PersistentMappingBuffer.h"
#include "video/Texture.h"

TestBloom::TestBloom(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testbloom");
	setCameraMotion(false);
	_allowRelativeMouseMode = false;
}

app::AppState TestBloom::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	setUICamera();

	if (!_blurRenderer.init(false)) {
		Log::error("Failed to initialize the blur renderer");
		return app::AppState::InitFailure;
	}

	if (!_bloomRenderer.init(false)) {
		Log::error("Failed to initialize the bloom renderer");
		return app::AppState::InitFailure;
	}

	const image::ImagePtr& sceneImg = image::loadImage("bloom_scene", false);
	if (!sceneImg->isLoaded()) {
		Log::error("Failed to load the image for the scene");
		return app::AppState::InitFailure;
	}
	_sceneTexture = video::createTextureFromImage(sceneImg);
	if (!_sceneTexture) {
		Log::error("Failed to create texture for the scene");
		return app::AppState::InitFailure;
	}

	const image::ImagePtr& bloomImg = image::loadImage("bloom_extracted", false);
	if (!bloomImg->isLoaded()) {
		Log::error("Failed to load the image for the bloom");
		return app::AppState::InitFailure;
	}
	_bloomTexture = video::createTextureFromImage(bloomImg);
	if (!_bloomTexture) {
		Log::error("Failed to create texture for the bloom");
		return app::AppState::InitFailure;
	}

	return state;
}

app::AppState TestBloom::onCleanup() {
	_blurRenderer.shutdown();
	_bloomRenderer.shutdown();
	if (_bloomTexture) {
		_bloomTexture->shutdown();
	}
	if (_sceneTexture) {
		_sceneTexture->shutdown();
	}
	app::AppState state = Super::onCleanup();
	return state;
}

void TestBloom::onRenderUI() {
	Super::onRenderUI();

	static glm::ivec2 size(256, 256);

	if (ImGui::InputInt("blur passes: ", &_passes)) {
		_passes = glm::clamp(_passes, 1, 10);
	}

	ImGui::Image(_bloomTexture->handle(), size);

	ImGui::Text("scene");
	ImGui::Image(_sceneTexture->handle(), size);

	ImGui::Text("bloom raw");
	ImGui::Image(_bloomTexture->handle(), size);

	const video::TexturePtr& blurred = _blurRenderer.texture();
	ImGui::Text("blurred bloom: %i:%i", blurred->width(), blurred->height());
	ImGui::Image(blurred->handle(), size);
}

void TestBloom::doRender() {
	_blurRenderer.render(_bloomTexture->handle(), _passes);
	_bloomRenderer.render(_sceneTexture->handle(), _blurRenderer.texture()->handle());
}

TEST_APP(TestBloom)
