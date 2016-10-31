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
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (ev.target->GetID() == TBIDC("resetcamera")) {
			_scene->resetCamera();
			return true;
		} else if (ev.target->GetID() == TBIDC("actionoverride")) {
			_scene->setAction(EditorScene::Action::OverrideVoxel);
			return true;
		} else if (ev.target->GetID() == TBIDC("actiondelete")) {
			_scene->setAction(EditorScene::Action::DeleteVoxel);
			return true;
		} else if (ev.target->GetID() == TBIDC("actioncopy")) {
			_scene->setAction(EditorScene::Action::CopyVoxel);
			return true;
		} else if (ev.target->GetID() == TBIDC("actionplace")) {
			_scene->setAction(EditorScene::Action::PlaceVoxel);
			return true;
		}
	}

	if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (tb::TBSelectDropdown *select = GetWidgetByIDAndType<tb::TBSelectDropdown>(TBIDC("cammode"))) {
			const int value = select->GetValue();
			video::PolygonMode mode = video::PolygonMode::Solid;
			switch (value) {
			case 1:
				mode = video::PolygonMode::Points;
				break;
			case 2:
				mode = video::PolygonMode::WireFrame;
				break;
			default:
			case 0:
				break;
			}
			_scene->camera().setPolygonMode(mode);
		}
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
