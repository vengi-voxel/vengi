#include "MainWindow.h"
#include "editorscene/EditorScene.h"
#include "palette/PaletteWidget.h"
#include "../VoxEdit.h"
#include <assimp/Exporter.hpp>

namespace voxedit {

static const struct {
	tb::TBID id;
	Action action;
	bool availableOnEmpty;
} actions[] = {
	{TBIDC("actionoverride"),	Action::OverrideVoxel, false},
	{TBIDC("actiondelete"),		Action::DeleteVoxel, false},
	{TBIDC("actioncopy"),		Action::CopyVoxel, false},
	{TBIDC("actionplace"),		Action::PlaceVoxel, true},
	{TBIDC("actionselect"),		Action::SelectVoxels, false}
};

static const struct {
	tb::TBID id;
	SelectType type;
} selectionmodes[] = {
	{TBIDC("actionselectsingle"),		SelectType::Single},
	{TBIDC("actionselectsame"),			SelectType::Same},
	{TBIDC("actionselecthorizontal"),	SelectType::LineHorizontal},
	{TBIDC("actionselectvertical"),		SelectType::LineVertical},
	{TBIDC("actionselectedge"),			SelectType::Edge}
};

static const struct {
	tb::TBID id;
	Shape shape;
} shapes[] = {
	{TBIDC("shapecone"), Shape::Cone},
	{TBIDC("shapesingle"), Shape::Single},
	{TBIDC("shapesphere"), Shape::Sphere},
	{TBIDC("shapecircle"), Shape::Circle},
	{TBIDC("shapedome"), Shape::Dome},
	{TBIDC("shapeplane"), Shape::Plane}
};

MainWindow::MainWindow(VoxEdit* tool) :
		ui::Window(tool), _scene(nullptr), _voxedit(tool), _paletteWidget(nullptr) {
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

	_paletteWidget = getWidgetByType<PaletteWidget>("palettecontainer");
	if (_paletteWidget == nullptr) {
		Log::error("Failed to init the main window: Could not get the editor scene node with id 'palettecontainer'");
		return false;
	}

	_sceneTop = getWidgetByType<EditorScene>("editorscenetop");
	_sceneLeft = getWidgetByType<EditorScene>("editorsceneleft");
	_sceneFront = getWidgetByType<EditorScene>("editorscenefront");

	_fourViewAvailable = _sceneTop != nullptr && _sceneLeft != nullptr && _sceneFront != nullptr;

	tb::TBWidget* toggleViewPort = getWidget("toggleviewport");
	if (toggleViewPort != nullptr) {
		toggleViewPort->SetState(tb::WIDGET_STATE_DISABLED, !_fourViewAvailable);
	}
	_exportButton = getWidget("export");
	_saveButton = getWidget("save");
	_undoButton = getWidget("undo");
	_redoButton = getWidget("redo");

	_showAABB = GetWidgetByIDAndType<tb::TBCheckBox>(TBIDC("optionshowaabb"));
	_showGrid = GetWidgetByIDAndType<tb::TBCheckBox>(TBIDC("optionshowgrid"));
	_showAxis = GetWidgetByIDAndType<tb::TBCheckBox>(TBIDC("optionshowaxis"));
	_freeLook = GetWidgetByIDAndType<tb::TBCheckBox>(TBIDC("optionfreelook"));
	if (_showAABB == nullptr || _showGrid == nullptr || _showAxis == nullptr || _freeLook == nullptr) {
		Log::error("Could not load all required widgets");
		return false;
	}

	tb::TBWidget *viewPortToggleWidget = getWidget("toggleviewport");
	if (viewPortToggleWidget != nullptr) {
		const int value = viewPortToggleWidget->GetValue();
		const tb::WIDGET_VISIBILITY vis = value ? tb::WIDGET_VISIBILITY_VISIBLE : tb::WIDGET_VISIBILITY_GONE;
		if (_sceneTop != nullptr) {
			_sceneTop->SetVisibility(vis);
		}
		if (_sceneLeft != nullptr) {
			_sceneLeft->SetVisibility(vis);
		}
		if (_sceneFront != nullptr) {
			_sceneFront->SetVisibility(vis);
		}
	}

	_showAABB->SetValue(_scene->renderAABB() ? 1 : 0);
	_showGrid->SetValue(_scene->renderGrid() ? 1 : 0);
	_showAxis->SetValue(_scene->renderAxis() ? 1 : 0);
	_freeLook->SetValue(_scene->camera().rotationType() == video::CameraRotationType::Eye ? 1 : 0);

	Assimp::Exporter exporter;
	const size_t num = exporter.GetExportFormatCount();
	for (size_t i = 0; i < num; ++i) {
		const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
		_exportFilter.append(desc->fileExtension);
		if (i < num - 1) {
			_exportFilter.append(";");
		}
	}

	return true;
}

void MainWindow::toggleQuadViewport() {
	bool vis = false;
	if (_sceneTop != nullptr) {
		vis = _sceneTop->GetVisibilityCombined();
	}
	if (!vis && _sceneLeft != nullptr) {
		vis = _sceneLeft->GetVisibilityCombined();
	}
	if (!vis && _sceneFront != nullptr) {
		vis = _sceneFront->GetVisibilityCombined();
	}

	setQuadViewport(!vis);
}

void MainWindow::setQuadViewport(bool active) {
	const tb::WIDGET_VISIBILITY vis = active ? tb::WIDGET_VISIBILITY_VISIBLE : tb::WIDGET_VISIBILITY_GONE;
	if (_sceneTop != nullptr) {
		_sceneTop->SetVisibility(vis);
	}
	if (_sceneLeft != nullptr) {
		_sceneLeft->SetVisibility(vis);
	}
	if (_sceneFront != nullptr) {
		_sceneFront->SetVisibility(vis);
	}
	tb::TBWidget* toggleViewPort = getWidget("toggleviewport");
	if (toggleViewPort != nullptr) {
		toggleViewPort->SetValue(active ? 1 : 0);
	}
}

bool MainWindow::handleClickEvent(const tb::TBWidgetEvent &ev) {
	if (ev.target->GetID() == TBIDC("unsaved_changes_new")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			_scene->newModel(true);
			resetCameras();
		}
		return true;
	} else if (ev.target->GetID() == TBIDC("unsaved_changes_quit")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			Close();
		}
		return true;
	} else if (ev.target->GetID() == TBIDC("unsaved_changes_load")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			_scene->loadModel(_loadFile);
			resetCameras();
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
	} else if (ev.target->GetID() == TBIDC("quit")) {
		quit();
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
		_scene->setRenderGrid(ev.target->GetValue() == 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("optionshowaxis")) {
		_scene->setRenderAxis(ev.target->GetValue() == 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("optionshowaabb")) {
		_scene->setRenderAABB(ev.target->GetValue() == 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("optionfreelook")) {
		const int v = ev.target->GetValue();
		video::Camera& c = _scene->camera();
		if (v) {
			c.setRotationType(video::CameraRotationType::Eye);
		} else {
			c.setRotationType(video::CameraRotationType::Target);
		}
		return true;
	}
	for (uint32_t i = 0; i < SDL_arraysize(actions); ++i) {
		if (ev.target->GetID() == actions[i].id) {
			_scene->setAction(actions[i].action);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(selectionmodes); ++i) {
		if (ev.target->GetID() == selectionmodes[i].id) {
			_scene->setSelectionType(selectionmodes[i].type);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(shapes); ++i) {
		if (ev.target->GetID() == shapes[i].id) {
			_scene->setCursorShape(shapes[i].shape);
			return true;
		}
	}
	return false;
}

bool MainWindow::handleChangeEvent(const tb::TBWidgetEvent &ev) {
	if (ev.target->GetID() == TBIDC("cammode")) {
		tb::TBWidget *widget = ev.target;
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
	} else if (ev.target->GetID() == TBIDC("toggleviewport")) {
		tb::TBWidget *widget = ev.target;
		const int value = widget->GetValue();
		setQuadViewport(value == 1);
		return true;
	}

	return false;
}

void MainWindow::OnProcess() {
	Super::OnProcess();
	if (_paletteWidget->isDirty()) {
		_scene->setVoxelType(_paletteWidget->voxelType());
		_paletteWidget->markAsClean();
	}
	const bool empty = _scene->isEmpty();
	if (_exportButton != nullptr) {
		_exportButton->SetState(tb::WIDGET_STATE_DISABLED, empty);
	}
	if (_saveButton != nullptr) {
		_saveButton->SetState(tb::WIDGET_STATE_DISABLED, empty);
	}
	if (_undoButton != nullptr) {
		_undoButton->SetState(tb::WIDGET_STATE_DISABLED, empty);
	}
	if (_redoButton != nullptr) {
		_redoButton->SetState(tb::WIDGET_STATE_DISABLED, empty);
	}
	for (uint32_t i = 0; i < SDL_arraysize(actions); ++i) {
		tb::TBWidget* w = GetWidgetByID(actions[i].id);
		if (w == nullptr) {
			continue;
		}
		if (!actions[i].availableOnEmpty && empty) {
			if (w->GetState(tb::WIDGET_STATE_SELECTED)) {
				w->SetState(tb::WIDGET_STATE_SELECTED, false);
			}
			w->SetState(tb::WIDGET_STATE_DISABLED, true);
		} else {
			w->SetState(tb::WIDGET_STATE_DISABLED, false);
		}
	}
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

void MainWindow::OnDie() {
	Super::OnDie();
	_app->requestQuit();
}

void MainWindow::quit() {
	if (_scene->isDirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and quit?",
				ui::Window::PopupType::YesNo, "unsaved_changes_quit");
		return;
	}
	 Close();
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
		if (_scene->isEmpty()) {
			return false;
		}
		if (_exportFilter.empty()) {
			return false;
		}
		f = _voxedit->saveDialog(_exportFilter);
		if (f.empty()) {
			return false;
		}
		file = f;
	}
	return _scene->exportModel(file);
}

void MainWindow::resetCameras() {
	_scene->resetCamera();
	if (_sceneTop != nullptr) {
		_sceneTop->resetCamera();
	}
	if (_sceneLeft != nullptr) {
		_sceneLeft->resetCamera();
	}
	if (_sceneFront != nullptr) {
		_sceneFront->resetCamera();
	}
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
		if (_scene->loadModel(file)) {
			resetCameras();
			return true;
		}
		return false;
	}

	_loadFile = std::string(file);
	popup("Unsaved Modifications",
			"There are unsaved modifications.\nDo you wish to discard them and load?",
			ui::Window::PopupType::YesNo, "unsaved_changes_load");
	return false;
}

void MainWindow::select(const glm::ivec3& pos) {
	_scene->select(pos);
}

bool MainWindow::createNew(bool force) {
	if (!force && _scene->isDirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and close?",
				ui::Window::PopupType::YesNo, "unsaved_changes_new");
	}
	if (_scene->newModel(force)) {
		resetCameras();
		return true;
	}
	return false;
}

}
