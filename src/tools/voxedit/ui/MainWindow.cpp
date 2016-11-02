#include "MainWindow.h"
#include "EditorScene.h"
#include "../VoxEdit.h"

MainWindow::MainWindow(VoxEdit* tool) :
		ui::Window(tool), _scene(nullptr), _voxedit(tool) {
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

	_showAABB = GetWidgetByIDAndType<tb::TBCheckBox>(TBIDC("optionshowaabb"));
	_showGrid = GetWidgetByIDAndType<tb::TBCheckBox>(TBIDC("optionshowgrid"));
	_showAxis = GetWidgetByIDAndType<tb::TBCheckBox>(TBIDC("optionshowaxis"));
	if (_showAABB == nullptr || _showGrid == nullptr || _showAxis == nullptr) {
		Log::error("Could not load all required widgets");
		return false;
	}

	_showAABB->SetValue(_scene->renderAABB() ? 1 : 0);
	_showGrid->SetValue(_scene->renderGrid() ? 1 : 0);
	_showAxis->SetValue(_scene->renderAxis() ? 1 : 0);

	return true;
}

static const struct {
	tb::TBID id;
	EditorScene::Action action;
} actions[] = {
	{TBIDC("actionoverride"),	EditorScene::Action::OverrideVoxel},
	{TBIDC("actiondelete"),		EditorScene::Action::DeleteVoxel},
	{TBIDC("actioncopy"),		EditorScene::Action::CopyVoxel},
	{TBIDC("actionplace"),		EditorScene::Action::PlaceVoxel},
};

bool MainWindow::handleClickEvent(const tb::TBWidgetEvent &ev) {
	if (ev.target->GetID() == TBIDC("unsaved_changes_new")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			_scene->newModel(true);
		}
		return true;
	} else if (ev.target->GetID() == TBIDC("unsaved_changes_load")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			_scene->loadModel(_loadFile);
		}
		return true;
	} else if (ev.target->GetID() == TBIDC("unsaved_changes_voxelize")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			const video::MeshPtr& mesh = _voxedit->meshPool()->getMesh(_voxelizeFile, false);
			_scene->voxelizeModel(mesh);
		}
		return true;
	}

	if (ev.target->GetID() == TBIDC("resetcamera")) {
		_scene->resetCamera();
		return true;
	} else if (ev.target->GetID() == TBIDC("new")) {
		createNew(false);
		return true;
	} else if (ev.target->GetID() == TBIDC("load")) {
		load("");
		return true;
	} else if (ev.target->GetID() == TBIDC("export")) {
		exportFile("");
		return true;
	} else if (ev.target->GetID() == TBIDC("save")) {
		save("");
		return true;
	} else if (ev.target->GetID() == TBIDC("optionshowgrid")) {
		_scene->setRenderGrid(_showGrid->GetValue() == 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("optionshowaxis")) {
		_scene->setRenderAxis(_showAxis->GetValue() == 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("optionshowaabb")) {
		_scene->setRenderAABB(_showAABB->GetValue() == 1);
		return true;
	}
	for (uint32_t i = 0; i < SDL_arraysize(actions); ++i) {
		if (ev.target->GetID() == actions[i].id) {
			_scene->setAction(actions[i].action);
			return true;
		}
	}
	return false;
}

bool MainWindow::handleChangeEvent(const tb::TBWidgetEvent &ev) {
	if (ev.target->GetID() == TBIDC("cammode")) {
		tb::TBSelectDropdown *widget = GetWidgetByIDAndType<tb::TBSelectDropdown>(TBIDC("cammode"));
		if (widget == nullptr) {
			return false;
		}
		const int value = widget->GetValue();
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
		return true;
	}
	return false;
}

bool MainWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (handleClickEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (handleChangeEvent(ev)) {
			return true;
		}
	}

	return ui::Window::OnEvent(ev);
}

bool MainWindow::save(std::string_view file) {
	if (file.empty()) {
		const std::string& f = _voxedit->saveDialog("vox,qbt");
		if (f.empty()) {
			return false;
		}
		return _scene->saveModel(f);
	}

	return _scene->saveModel(file);
}

bool MainWindow::voxelize(std::string_view file) {
	std::string f;
	if (file.empty()) {
		f = _voxedit->openDialog("vox,qbt");
		if (f.empty()) {
			return false;
		}
		file = f;
	}

	if (!_scene->isDirty()) {
		const video::MeshPtr& mesh = _voxedit->meshPool()->getMesh(file, false);
		return _scene->voxelizeModel(mesh);
	}

	_voxelizeFile = std::string(file);
	popup("Unsaved Modifications",
			"There are unsaved modifications.\nDo you wish to discard them and start the voxelize process?",
			ui::Window::PopupType::YesNo, "unsaved_changes_voxelize");
	return false;
}

bool MainWindow::exportFile(std::string_view file) {
	std::string f;
	if (file.empty()) {
		// TODO: extract extension - filter
		f = _voxedit->openDialog("vox,qbt");
		if (f.empty()) {
			return false;
		}
		file = f;
	}
	return _scene->exportModel(file);
}

bool MainWindow::load(std::string_view file) {
	std::string f;
	if (file.empty()) {
		f = _voxedit->openDialog("vox,qbt");
		if (f.empty()) {
			return false;
		}
		file = f;
	}

	if (!_scene->isDirty()) {
		return _scene->loadModel(file);
	}

	_loadFile = std::string(file);
	popup("Unsaved Modifications",
			"There are unsaved modifications.\nDo you wish to discard them and load?",
			ui::Window::PopupType::YesNo, "unsaved_changes_load");
	return false;
}

bool MainWindow::createNew(bool force) {
	if (!force && _scene->isDirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and close?",
				ui::Window::PopupType::YesNo, "unsaved_changes_new");
	}
	return _scene->newModel(force);
}
