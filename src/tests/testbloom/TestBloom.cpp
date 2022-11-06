/**
 * @file
 */

#include "TestBloom.h"
#include "image/Image.h"
#include "imgui.h"
#include "testcore/TestAppMain.h"
#include "video/Texture.h"
#include "core/Log.h"

TestBloom::TestBloom(const metric::MetricPtr &metric, const io::FilesystemPtr &filesystem,
					 const core::TimeProviderPtr &timeProvider)
	: Super(metric, filesystem, timeProvider) {
	init(ORGANISATION, "TestBloom");
	setCameraMotion(false);
	_allowRelativeMouseMode = false;
}

app::AppState TestBloom::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	setUICamera();

	const image::ImagePtr &sceneImg = image::loadImage("bloom_scene", false);
	if (!sceneImg->isLoaded()) {
		Log::error("Failed to load the image for the scene");
		return app::AppState::InitFailure;
	}

	if (!_bloomRenderer.init(false, sceneImg->width(), sceneImg->height())) {
		Log::error("Failed to initialize the blur renderer");
		return app::AppState::InitFailure;
	}
	_sceneTexture = video::createTextureFromImage(sceneImg);
	if (!_sceneTexture) {
		Log::error("Failed to create texture for the scene");
		return app::AppState::InitFailure;
	}

	const image::ImagePtr &glowImg = image::loadImage("bloom_extracted", false);
	if (!glowImg->isLoaded()) {
		Log::error("Failed to load the image for the glow");
		return app::AppState::InitFailure;
	}

	_glowTexture = video::createTextureFromImage(glowImg);
	if (!_glowTexture) {
		Log::error("Failed to create texture for the glow");
		return app::AppState::InitFailure;
	}

	return state;
}

app::AppState TestBloom::onCleanup() {
	_bloomRenderer.shutdown();
	if (_sceneTexture) {
		_sceneTexture->shutdown();
	}
	app::AppState state = Super::onCleanup();
	return state;
}

void TestBloom::onRenderUI() {
	Super::onRenderUI();

	ImGui::Text("scene");
	ImGui::Image(_sceneTexture->handle(), glm::ivec2(_sceneTexture->width(), _sceneTexture->height()));

	ImGui::Text("glow");
	ImGui::Image(_glowTexture->handle(), glm::ivec2(_glowTexture->width(), _glowTexture->height()));

	for (int i = 0; i < render::BloomRenderer::passes(); ++i) {
		ImGui::Separator();
		{
			const video::TexturePtr &tex = _bloomRenderer.texture0(i);
			ImGui::Text("texture0[%i] %i:%i", i, tex->width(), tex->height());
			ImGui::Image(tex->handle(), glm::ivec2(tex->width(), tex->height()));
		}
		{
			const video::TexturePtr &tex = _bloomRenderer.texture1(i);
			ImGui::Text("texture1[%i] %i:%i", i, tex->width(), tex->height());
			ImGui::Image(tex->handle(), glm::ivec2(tex->width(), tex->height()));
		}
		{
			const video::TexturePtr &tex = _bloomRenderer.texture2(i);
			ImGui::Text("texture2[%i] %i:%i", i, tex->width(), tex->height());
			ImGui::Image(tex->handle(), glm::ivec2(tex->width(), tex->height()));
		}
	}
}

void TestBloom::doRender() {
	_bloomRenderer.render(_sceneTexture, _glowTexture);
}

TEST_APP(TestBloom)
