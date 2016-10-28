/**
 * @file
 */

#include "VoxEdit.h"
#include "Actions.h"
#include "ui/MainWindow.h"
#include "core/Color.h"
#include "core/Command.h"
#include "video/GLFunc.h"
#include "ui/EditorScene.h"

VoxEdit::VoxEdit(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		ui::UIApp(filesystem, eventBus, timeProvider) {
	init("engine", "voxedit");
}

bool VoxEdit::saveFile(std::string_view file) {
	EditorScene* scene = getWidgetByType<EditorScene>("editorscene");
	if (scene == nullptr) {
		Log::error("Failed to save: Could not get the editor scene node");
		return false;
	}
	return scene->saveModel(file);
}

bool VoxEdit::loadFile(std::string_view file) {
	EditorScene* scene = getWidgetByType<EditorScene>("editorscene");
	if (scene == nullptr) {
		Log::error("Failed to load: Could not get the editor scene node");
		return false;
	}
	return scene->loadModel(file);
}

bool VoxEdit::newFile(bool force) {
	EditorScene* scene = getWidgetByType<EditorScene>("editorscene");
	if (scene == nullptr) {
		Log::error("Failed to create new scene: Could not get the editor scene node");
		return false;
	}
	return scene->newModel(force);
}

core::AppState VoxEdit::onInit() {
	const core::AppState state = Super::onInit();

	_lastDirectory = core::Var::get("ve_lastdirectory", core::App::getInstance()->filesystem()->homePath().c_str());

	new MainWindow(this);

	ui::Widget* widget = getWidget("editorcontainer");
	if (widget == nullptr) {
		Log::error("Could not get editorcontainer widget from ui definition");
		return core::AppState::Cleanup;
	}

	registerActions(this, _lastDirectory);

	// TODO: if tmpfile exists, load that one
	newFile(true);

	const glm::vec4& color = ::core::Color::Black;
	glClearColor(color.r, color.g, color.b, color.a);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	setRelativeMouseMode(false);

	return state;
}

core::AppState VoxEdit::onRunning() {
	core::AppState state = Super::onRunning();
	if (state == core::AppState::Cleanup) {
		return state;
	}
	const bool current = isRelativeMouseMode();
	if (current) {
		centerMouseCursor();
	}

	return state;
}

core::AppState VoxEdit::onCleanup() {
	core::Command::unregisterCommand("+move_right");
	core::Command::unregisterCommand("+move_left");
	core::Command::unregisterCommand("+move_upt");
	core::Command::unregisterCommand("+move_down");

	// saveFile(tmpFilename);
	// TODO: cvar with tmpFilename to load on next start

	return Super::onCleanup();
}

bool VoxEdit::onKeyPress(int32_t key, int16_t modifier) {
	if (Super::onKeyPress(key, modifier)) {
		return true;
	}
	if (key == SDLK_ESCAPE) {
		toggleRelativeMouseMode();
		return true;
	}
	return false;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	VoxEdit app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
