/**
 * @file
 */

#include "TestBlur.h"
#include "image/Image.h"
#include "imgui.h"
#include "testcore/TestAppMain.h"
#include "video/Texture.h"

TestBlur::TestBlur(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "TestBlur");
	setCameraMotion(false);
	_allowRelativeMouseMode = false;
}

app::AppState TestBlur::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	setUICamera();

	if (!_blurRenderer.init(false)) {
		Log::error("Failed to initialize the blur renderer");
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

	return state;
}

app::AppState TestBlur::onCleanup() {
	_blurRenderer.shutdown();
	if (_sceneTexture) {
		_sceneTexture->shutdown();
	}
	app::AppState state = Super::onCleanup();
	return state;
}

void TestBlur::onRenderUI() {
	Super::onRenderUI();

	static glm::ivec2 size(512, 512);

	if (ImGui::InputInt("blur passes: ", &_passes)) {
		_passes = glm::clamp(_passes, 0, 20);
	}

	ImGui::Text("scene");
	ImGui::Image(_sceneTexture->handle(), size);

	const video::TexturePtr& blurred = _blurRenderer.texture();
	ImGui::Text("blurred: %i:%i", blurred->width(), blurred->height());
	ImGui::Image(blurred->handle(), size);
}

void TestBlur::doRender() {
	_blurRenderer.render(_sceneTexture->handle(), _passes);
}

TEST_APP(TestBlur)
