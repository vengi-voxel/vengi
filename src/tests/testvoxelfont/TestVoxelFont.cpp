#include "TestVoxelFont.h"

TestVoxelFont::TestVoxelFont(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider) {
	setCameraMotion(true);
}

core::AppState TestVoxelFont::onInit() {
	core::AppState state = Super::onInit();

	if (!_rawVolumeRenderer.init()) {
		Log::error("Failed to initialize the raw volume renderer");
		return core::AppState::Cleanup;
	}
	if (!_rawVolumeRenderer.onResize(glm::ivec2(), dimension())) {
		Log::error("Failed to initialize the raw volume renderer");
		return core::AppState::Cleanup;
	}

	if (!changeFontSize(0)) {
		Log::error("Failed to start voxel font test application - could not load the given font file");
		return core::AppState::Cleanup;
	}

	_camera.setFarPlane(4000.0f);

	return state;
}

core::AppState TestVoxelFont::onCleanup() {
	core::AppState state = Super::onCleanup();
	_voxelFont.shutdown();
	voxel::RawVolume* old = _rawVolumeRenderer.shutdown();
	delete old;
	return state;
}

bool TestVoxelFont::changeFontSize(int delta) {
	_vertices = 0;
	_indices = 0;
	_voxelFont.shutdown();
	_fontSize = glm::clamp(_fontSize + delta, 2, 250);
	if (!_voxelFont.init("font.ttf", _fontSize, _thickness, _mergeQuads, " Helowrd!")) {
		return false;
	}

	std::vector<voxel::Vertex> vertices;
	std::vector<voxel::IndexType> indices;

	const char* str = "Hello world!";
	const int renderedChars = _voxelFont.render(str, vertices, indices);
	if ((int)strlen(str) != renderedChars) {
		Log::error("Failed to render string '%s' (chars: %i)", str, renderedChars);
		return false;
	}

	if (indices.empty() || vertices.empty()) {
		Log::error("Failed to render voxel font");
		return false;
	}

	if (!_rawVolumeRenderer.update(vertices, indices)) {
		return false;
	}
	_vertices = vertices.size();
	_indices = indices.size();

	return true;
}

void TestVoxelFont::onMouseWheel(int32_t x, int32_t y) {
	const SDL_Keymod mods = SDL_GetModState();
	if (mods & KMOD_SHIFT) {
		changeFontSize(y);
		return;
	}

	Super::onMouseWheel(x, y);
}

bool TestVoxelFont::onKeyPress(int32_t key, int16_t modifier) {
	const bool retVal = Super::onKeyPress(key, modifier);
	if (modifier & KMOD_SHIFT) {
		int delta = 0;
		if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
			delta = -1;
		} else if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
			delta = 1;
		}

		if (delta != 0) {
			changeFontSize(delta);
			return true;
		}
	}
	if (modifier & KMOD_CTRL) {
		int delta = 0;
		if (key == SDLK_MINUS || key == SDLK_KP_MINUS) {
			delta = -1;
		} else if (key == SDLK_PLUS || key == SDLK_KP_PLUS) {
			delta = 1;
		}

		if (delta != 0) {
			_thickness = glm::clamp(_thickness + delta, 1, 250);
			changeFontSize(0);
			return true;
		}
	}
	if (key == SDLK_SPACE) {
		_mergeQuads ^= true;
		changeFontSize(0);
		return true;
	}

	return retVal;
}

void TestVoxelFont::afterRootWidget() {
	enqueueShowStr(5, core::Color::White, "Fontsize: %i", _fontSize);
	enqueueShowStr(5, core::Color::White, "Thickness: %i", _thickness);
	const char *state = _mergeQuads ? "true" : "false";
	enqueueShowStr(5, core::Color::White, "Merge Quads: %s", state);
	enqueueShowStr(5, core::Color::White, "Font vertices: %i, indices: %i", _vertices, _indices);
	enqueueShowStr(5, core::Color::Gray, "Ctrl/+ Ctrl/-: Change font thickness");
	enqueueShowStr(5, core::Color::Gray, "Space: Toggle merge quads");
	enqueueShowStr(5, core::Color::Gray, "Shift/+ Shift/-: Change font size");
	enqueueShowStr(5, core::Color::Gray, "Shift/Mousewheel: Change font size");
	Super::afterRootWidget();
}

void TestVoxelFont::doRender() {
	_rawVolumeRenderer.render(_camera);
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	TestVoxelFont app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
