#include "MainWindow.h"
#include "EditorScene.h"

MainWindow::MainWindow(ui::UIApp* tool) :
		ui::Window(tool), _scene(nullptr) {
	SetSettings(tb::WINDOW_SETTINGS_CAN_ACTIVATE);
}

bool MainWindow::init() {
	if (!loadResourceFile("ui/window/main.tb.txt")) {
		Log::error("Failed to init the main window: Could not load the ui definition");
		return false;
	}
	_scene = getWidgetByType<EditorScene>("editorscene");
	if (_scene == nullptr) {
		Log::error("Failed to init the main window: Could not get the editor scene node with id 'editorscene'");
		return false;
	}
	return true;
}

bool MainWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK && ev.target->GetID() == TBIDC("resetcamera")) {
		_scene->resetCamera();
		return true;
	}
	return ui::Window::OnEvent(ev);
}

bool MainWindow::save(std::string_view file) {
	return _scene->saveModel(file);
}

bool MainWindow::load(std::string_view file) {
	return _scene->loadModel(file);
}

bool MainWindow::createNew(bool force) {
	return _scene->newModel(force);
}
