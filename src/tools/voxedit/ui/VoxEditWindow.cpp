#include "VoxEditWindow.h"
#include "LSystemWindow.h"
#include "NoiseWindow.h"
#include "WorldWindow.h"
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
	{TBIDC("shapecone"),	Shape::Cone},
	{TBIDC("shapesingle"),	Shape::Single},
	{TBIDC("shapesphere"),	Shape::Sphere},
	{TBIDC("shapecircle"),	Shape::Circle},
	{TBIDC("shapedome"),	Shape::Dome},
	{TBIDC("shapeplane"),	Shape::Plane}
};

static const struct {
	const char *name;
	const char *id;
	tb::TBID tbid;
	voxel::TreeType type;
} treeTypes[] = {
	{"Pine",		"tree_pine",		TBIDC("tree_pine"),			voxel::TreeType::Pine},
	{"Dome",		"tree_dome",		TBIDC("tree_dome"),			voxel::TreeType::Dome},
	{"Dome Hanging","tree_dome2",		TBIDC("tree_dome2"),		voxel::TreeType::DomeHangingLeaves},
	{"Cone",		"tree_cone",		TBIDC("tree_cone"),			voxel::TreeType::Cone},
	{"Fir",			"tree_fir",			TBIDC("tree_fir"),			voxel::TreeType::Fir},
	{"Ellipsis2",	"tree_ellipsis2",	TBIDC("tree_ellipsis2"),	voxel::TreeType::BranchesEllipsis},
	{"Ellipsis",	"tree_ellipsis",	TBIDC("tree_ellipsis"),		voxel::TreeType::Ellipsis},
	{"Cube",		"tree_cube",		TBIDC("tree_cube"),			voxel::TreeType::Cube},
	{"Cube Sides",	"tree_cube2",		TBIDC("tree_cube2"),		voxel::TreeType::CubeSideCubes}
};
static_assert((int)SDL_arraysize(treeTypes) == (int)voxel::TreeType::Max, "Missing support for tree types in the ui");

VoxEditWindow::VoxEditWindow(VoxEdit* tool) :
		ui::Window(tool), _scene(nullptr), _voxedit(tool), _paletteWidget(nullptr) {
	SetSettings(tb::WINDOW_SETTINGS_CAN_ACTIVATE);
}

VoxEditWindow::~VoxEditWindow() {
}

bool VoxEditWindow::init() {
	if (!loadResourceFile("ui/window/voxedit-main.tb.txt")) {
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
	_scene->setVoxelType(_paletteWidget->voxelType());
	_paletteWidget->markAsClean();

	_sceneTop = getWidgetByType<EditorScene>("editorscenetop");
	_sceneLeft = getWidgetByType<EditorScene>("editorsceneleft");
	_sceneFront = getWidgetByType<EditorScene>("editorscenefront");

	_fourViewAvailable = _sceneTop != nullptr && _sceneLeft != nullptr && _sceneFront != nullptr;

	tb::TBWidget* toggleViewPort = getWidget("toggleviewport");
	if (toggleViewPort != nullptr) {
		toggleViewPort->SetState(tb::WIDGET_STATE_DISABLED, !_fourViewAvailable);
		const int value = toggleViewPort->GetValue();
		setQuadViewport(value == 1);
	}
	_exportButton = getWidget("export");
	_saveButton = getWidget("save");
	_undoButton = getWidget("undo");
	_redoButton = getWidget("redo");

	_cursorX = getWidgetByType<tb::TBEditField>("cursorx");
	_cursorY = getWidgetByType<tb::TBEditField>("cursory");
	_cursorZ = getWidgetByType<tb::TBEditField>("cursorz");

	_lockedX = getWidgetByType<tb::TBCheckBox>("lockx");
	_lockedY = getWidgetByType<tb::TBCheckBox>("locky");
	_lockedZ = getWidgetByType<tb::TBCheckBox>("lockz");

	_showAABB = getWidgetByType<tb::TBCheckBox>("optionshowaabb");
	_showGrid = getWidgetByType<tb::TBCheckBox>("optionshowgrid");
	_showAxis = getWidgetByType<tb::TBCheckBox>("optionshowaxis");
	_freeLook = getWidgetByType<tb::TBCheckBox>("optionfreelook");
	if (_showAABB == nullptr || _showGrid == nullptr || _showAxis == nullptr || _freeLook == nullptr) {
		Log::error("Could not load all required widgets");
		return false;
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

void VoxEditWindow::addMenuItem(tb::TBSelectItemSourceList<tb::TBGenericStringItem>& items, const char *text, const char *id) {
	tb::TBGenericStringItem* item;
	if (id == nullptr) {
		const std::string& lowerId = core::string::toLower(text);
		item = new tb::TBGenericStringItem(text, TBIDC(lowerId.c_str()));
		const std::string& iconId = core::App::getInstance()->appname() + "-" + lowerId;
		item->SetSkinImage(TBIDC(iconId.c_str()));
	} else {
		item = new tb::TBGenericStringItem(text, TBIDC(id));
		char buf[128];
		SDL_snprintf(buf, sizeof(buf), "%s-%s", core::App::getInstance()->appname().c_str(), id);
		item->SetSkinImage(TBIDC(buf));
	}
	items.AddItem(item);
}

void VoxEditWindow::setCursorPosition(int x, int y, int z) {
	_scene->setCursorPosition(glm::ivec3(x, y, z), true);
}

void VoxEditWindow::rotate(int x, int y, int z) {
	Log::debug("execute rotate by %i:%i:%i", x, y, z);
	_scene->rotate(x, y, z);
}

void VoxEditWindow::scale(float x, float y, float z) {
	Log::debug("execute scale by %f:%f:%f", x, y, z);
	_scene->scaleCursorShape(glm::vec3(x, y, z));
}

void VoxEditWindow::move(int x, int y, int z) {
	Log::debug("execute move by %i:%i:%i", x, y, z);
	_scene->move(x, y, z);
}

void VoxEditWindow::executeMode() {
	if (_mode == ModifierMode::None) {
		_modeNumberBuf[0] = '\0';
		_lastModePress = -1l;
		_axis = voxedit::Axis::None;
		return;
	}

	if (_modeNumberBuf[0] != '\0') {
		if (_mode == ModifierMode::Scale) {
			const float value = core::string::toFloat(_modeNumberBuf);
			glm::vec3 values(1.0f, 1.0f, 1.0f);
			if ((_axis & voxedit::Axis::X) != voxedit::Axis::None) {
				values.x = value;
			}
			if ((_axis & voxedit::Axis::Y) != voxedit::Axis::None) {
				values.y = value;
			}
			if ((_axis & voxedit::Axis::Z) != voxedit::Axis::None) {
				values.z = value;
			}
			scale(values.x, values.y, values.z);
		} else {
			const int value = core::string::toInt(_modeNumberBuf);
			glm::ivec3 values(0, 0, 0);
			if ((_axis & voxedit::Axis::X) != voxedit::Axis::None) {
				values.x = value;
			}
			if ((_axis & voxedit::Axis::Y) != voxedit::Axis::None) {
				values.y = value;
			}
			if ((_axis & voxedit::Axis::Z) != voxedit::Axis::None) {
				values.z = value;
			}

			if (_mode == ModifierMode::Rotate) {
				rotate(values.x, values.y, values.z);
			} else if (_mode == ModifierMode::Move) {
				move(values.x, values.y, values.z);
			}
		}
	}
	if (_mode == ModifierMode::Lock) {
		const voxedit::Axis locked = _scene->lockedAxis();
#define VOXEDIT_LOCK(axis) if ((_axis & axis) != voxedit::Axis::None) { _scene->setLockedAxis(axis, (locked & axis) != voxedit::Axis::None); _lockedDirty = true; }
		VOXEDIT_LOCK(voxedit::Axis::X)
		VOXEDIT_LOCK(voxedit::Axis::Y)
		VOXEDIT_LOCK(voxedit::Axis::Z)
#undef VOXEDIT_LOCK
	}

	_modeNumberBuf[0] = '\0';
	_lastModePress = -1l;
	_axis = voxedit::Axis::None;
	_mode = ModifierMode::None;
}

void VoxEditWindow::toggleviewport() {
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

void VoxEditWindow::unselectall() {
	_scene->unselectAll();
}

void VoxEditWindow::rotatemode() {
	_mode = ModifierMode::Rotate;
	_axis = voxedit::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::scalemode() {
	_mode = ModifierMode::Scale;
	_axis = voxedit::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::movemode() {
	_mode = ModifierMode::Move;
	_axis = voxedit::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::togglelockaxis() {
	_mode = ModifierMode::Lock;
	_axis = voxedit::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::togglefreelook() {
	if (_freeLook == nullptr) {
		return;
	}
	const int v = _freeLook->GetValue();
	_freeLook->SetValue(v == 0 ? 1 : 0);
	video::Camera& c = _scene->camera();
	if (v == 0) {
		c.setRotationType(video::CameraRotationType::Eye);
	} else {
		c.setRotationType(video::CameraRotationType::Target);
	}
}

void VoxEditWindow::setQuadViewport(bool active) {
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

void VoxEditWindow::createTree(voxel::TreeType type) {
	voxel::TreeContext ctx;
	ctx.type = type;
	_scene->createTree(ctx);
}

static inline bool isAny(const tb::TBWidgetEvent& ev, const tb::TBID& id) {
	return ev.target->GetID() == id || ev.ref_id == id;
}

bool VoxEditWindow::handleEvent(const tb::TBWidgetEvent &ev) {
	if (isAny(ev, TBIDC("resetcamera"))) {
		_scene->resetCamera();
		return true;
	} else if (isAny(ev, TBIDC("quit"))) {
		quit();
		return true;
	} else if (isAny(ev, TBIDC("crop"))) {
		crop();
		return true;
	} else if (isAny(ev, TBIDC("extend"))) {
		extend();
		return true;
	} else if (isAny(ev, TBIDC("new"))) {
		createNew(false);
		return true;
	} else if (isAny(ev, TBIDC("load"))) {
		load("");
		return true;
	} else if (isAny(ev, TBIDC("export"))) {
		exportFile("");
		return true;
	} else if (isAny(ev, TBIDC("save"))) {
		save("");
		return true;
	} else if (isAny(ev, TBIDC("redo"))) {
		redo();
		return true;
	} else if (isAny(ev, TBIDC("undo"))) {
		undo();
		return true;
	} else if (isAny(ev, TBIDC("rotatex"))) {
		rotatex();
		return true;
	} else if (isAny(ev, TBIDC("rotatey"))) {
		rotatey();
		return true;
	} else if (isAny(ev, TBIDC("rotatez"))) {
		rotatez();
		return true;
	} else if (isAny(ev, TBIDC("menu_tree"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("tree_popup"))) {
			tb::TBSelectItemSourceList<tb::TBGenericStringItem>* treeItems = new tb::TBSelectItemSourceList<tb::TBGenericStringItem>();
			for (uint32_t i = 0; i < SDL_arraysize(treeTypes); ++i) {
				addMenuItem(*treeItems, treeTypes[i].name, treeTypes[i].id);
			}
			menu->Show(treeItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (isAny(ev, TBIDC("menu_file"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("tree_file"))) {
			tb::TBSelectItemSourceList<tb::TBGenericStringItem>* fileItems = new tb::TBSelectItemSourceList<tb::TBGenericStringItem>();
			addMenuItem(*fileItems, "New");
			addMenuItem(*fileItems, "Load");
			addMenuItem(*fileItems, "Save");
			addMenuItem(*fileItems, "Export");
			addMenuItem(*fileItems, "Quit");
			menu->Show(fileItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (isAny(ev, TBIDC("dialog_lsystem"))) {
		new LSystemWindow(this, _scene);
		return true;
	} else if (isAny(ev, TBIDC("dialog_noise"))) {
		new NoiseWindow(this, _scene);
		return true;
	} else if (isAny(ev, TBIDC("dialog_world"))) {
		new WorldWindow(this, _scene);
		return true;
	} else if (isAny(ev, TBIDC("optionshowgrid"))) {
		_scene->setRenderGrid(ev.target->GetValue() == 1);
		return true;
	} else if (isAny(ev, TBIDC("optionshowaxis"))) {
		_scene->setRenderAxis(ev.target->GetValue() == 1);
		return true;
	} else if (isAny(ev, TBIDC("optionshowaabb"))) {
		_scene->setRenderAABB(ev.target->GetValue() == 1);
		return true;
	} else if (isAny(ev, TBIDC("optionfreelook"))) {
		togglefreelook();
		return true;
	}
	return false;
}

bool VoxEditWindow::handleClickEvent(const tb::TBWidgetEvent &ev) {
	if (ev.target->GetID() == TBIDC("unsaved_changes_new")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			_scene->newModel(true);
			resetcamera();
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
			resetcamera();
		}
		return true;
	} else if (ev.target->GetID() == TBIDC("unsaved_changes_voxelize")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			const video::MeshPtr& mesh = _voxedit->meshPool()->getMesh(_voxelizeFile, false);
			_scene->voxelizeModel(mesh);
		}
		return true;
	}

	if (handleEvent(ev)) {
		return true;
	}

	for (uint32_t i = 0; i < SDL_arraysize(actions); ++i) {
		if (isAny(ev, actions[i].id)) {
			_scene->setAction(actions[i].action);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(selectionmodes); ++i) {
		if (isAny(ev, selectionmodes[i].id)) {
			_scene->setSelectionType(selectionmodes[i].type);
			setAction(Action::SelectVoxels);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(shapes); ++i) {
		if (isAny(ev, shapes[i].id)) {
			_scene->setCursorShape(shapes[i].shape);
			return true;
		}
	}
	for (uint32_t i = 0; i < SDL_arraysize(treeTypes); ++i) {
		if (isAny(ev, treeTypes[i].tbid)) {
			createTree(treeTypes[i].type);
			return true;
		}
	}
	Log::debug("Unknown event %s - %s", ev.target->GetID().debug_string.CStr(), ev.ref_id.debug_string.CStr());

	return false;
}

void VoxEditWindow::setSelectionType(SelectType type) {
	for (uint32_t i = 0; i < SDL_arraysize(selectionmodes); ++i) {
		if (selectionmodes[i].type != type) {
			continue;
		}
		tb::TBWidget* widget = GetWidgetByID(selectionmodes[i].id);
		if (widget != nullptr) {
			widget->SetValue(1);
		}
		_scene->setSelectionType(type);
		setAction(Action::SelectVoxels);
		break;
	}
}

void VoxEditWindow::setAction(Action action) {
	for (uint32_t i = 0; i < SDL_arraysize(actions); ++i) {
		if (actions[i].action != action) {
			continue;
		}
		if (_scene->isEmpty() && !actions[i].availableOnEmpty) {
			continue;
		}
		tb::TBWidget* widget = GetWidgetByID(actions[i].id);
		if (widget != nullptr) {
			widget->SetValue(1);
		}
		_scene->setAction(action);
		break;
	}
}

void VoxEditWindow::crop() {
	_scene->crop();
}

void VoxEditWindow::extend(int size) {
	_scene->extend(size);
}

bool VoxEditWindow::handleChangeEvent(const tb::TBWidgetEvent &ev) {
	if (ev.target->GetID() == TBIDC("cammode")) {
		tb::TBWidget *widget = ev.target;
		tb::TBWidget *parent = widget->GetParent();
		tb::TB_TYPE_ID typeId = GetTypeId<EditorScene>();
		if (!parent->IsOfTypeId(typeId)) {
			return false;
		}
		const int value = widget->GetValue();
		video::PolygonMode mode = video::PolygonMode::Solid;
		if (value == 1) {
			mode = video::PolygonMode::Points;
		} else if (value == 2) {
			mode = video::PolygonMode::WireFrame;
		}
		((EditorScene*)parent)->camera().setPolygonMode(mode);
		return true;
	} else if (ev.target->GetID() == TBIDC("toggleviewport")) {
		tb::TBWidget *widget = ev.target;
		const int value = widget->GetValue();
		setQuadViewport(value == 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("lockx")) {
		_scene->setLockedAxis(Axis::X, ev.target->GetValue() != 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("locky")) {
		_scene->setLockedAxis(Axis::Y, ev.target->GetValue() != 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("lockz")) {
		_scene->setLockedAxis(Axis::Z, ev.target->GetValue() != 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("cursorx")) {
		const tb::TBStr& str = ev.target->GetText();
		if (str.IsEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = _scene->cursorPosition();
		pos.x = val;
		_scene->setCursorPosition(pos, true);
		return true;
	} else if (ev.target->GetID() == TBIDC("cursory")) {
		const tb::TBStr& str = ev.target->GetText();
		if (str.IsEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = _scene->cursorPosition();
		pos.y = val;
		_scene->setCursorPosition(pos, true);
		return true;
	} else if (ev.target->GetID() == TBIDC("cursorz")) {
		const tb::TBStr& str = ev.target->GetText();
		if (str.IsEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = _scene->cursorPosition();
		pos.z = val;
		_scene->setCursorPosition(pos, true);
		return true;
	}

	return false;
}

void VoxEditWindow::OnProcess() {
	Super::OnProcess();

	if (_lastModePress > 0l && core::App::getInstance()->timeProvider()->tickTime() - _lastModePress > 1500l) {
		executeMode();
	}

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
		_undoButton->SetState(tb::WIDGET_STATE_DISABLED, empty || !_scene->canUndo());
	}
	if (_redoButton != nullptr) {
		_redoButton->SetState(tb::WIDGET_STATE_DISABLED, empty || !_scene->canRedo());
	}
	const glm::ivec3& pos = _scene->cursorPosition();
	if (_lastCursorPos != pos) {
		_lastCursorPos = pos;
		char buf[64];
		if (_cursorX != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.x);
			if (strcmp(_cursorX->GetText().CStr(), buf)) {
				_cursorX->SetText(buf);
			}
		}
		if (_cursorY != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.y);
			if (strcmp(_cursorY->GetText().CStr(), buf)) {
				_cursorY->SetText(buf);
			}
		}
		if (_cursorZ != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.z);
			if (strcmp(_cursorZ->GetText().CStr(), buf)) {
				_cursorZ->SetText(buf);
			}
		}
	}

	if (_lockedDirty) {
		_lockedDirty = false;
		Log::info("_lockedDirty = true;");
		const voxedit::Axis axis = _scene->lockedAxis();
		if (_lockedX != nullptr) {
			_lockedX->SetValue((axis & voxedit::Axis::X) != voxedit::Axis::None);
		}
		if (_lockedY != nullptr) {
			_lockedY->SetValue((axis & voxedit::Axis::Y) != voxedit::Axis::None);
		}
		if (_lockedZ != nullptr) {
			_lockedZ->SetValue((axis & voxedit::Axis::Z) != voxedit::Axis::None);
		}
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

static inline bool isValidNumberKey(int key) {
	if ((key >= SDLK_0 && key <= SDLK_9) || (key >= SDLK_KP_0 && key <= SDLK_KP_9)) {
		return true;
	}
	if (key == SDLK_PERIOD || key == SDLK_KP_PERIOD || key == SDLK_COMMA || key == SDLK_KP_COMMA) {
		return true;
	}
	if (key == SDLK_PLUS || key == SDLK_MINUS || key == SDLK_KP_PLUS || key == SDLK_KP_MINUS) {
		return true;
	}
	return false;
}

bool VoxEditWindow::OnEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CUSTOM) {
		if (handleEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (handleClickEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (handleChangeEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_SHORTCUT) {
		if (ev.ref_id == TBIDC("undo")) {
			undo();
		} else if (ev.ref_id == TBIDC("redo")) {
			redo();
		} else if (ev.ref_id == TBIDC("copy")) {
			copy();
		} else if (ev.ref_id == TBIDC("paste")) {
			paste();
		} else if (ev.ref_id == TBIDC("cut")) {
			cut();
		}
	} else if (ev.type == tb::EVENT_TYPE_KEY_DOWN) {
		const int key = ev.key;
		if (_axis != voxedit::Axis::None) {
			if (isValidNumberKey(key)) {
				int l = SDL_strlen(_modeNumberBuf);
				if (l < MODENUMBERBUFSIZE - 1) {
					_modeNumberBuf[l++] = (uint8_t)key;
					_modeNumberBuf[l] = '\0';
					_lastModePress = core::App::getInstance()->timeProvider()->tickTime();
				}
			} else if (ev.special_key == tb::TB_KEY_ENTER) {
				executeMode();
			}
		} else if (_mode != ModifierMode::None) {
			if (key == SDLK_x) {
				Log::debug("Set axis to x");
				_axis |= voxedit::Axis::X;
			} else if (key == SDLK_y) {
				_axis |= voxedit::Axis::Y;
				Log::debug("Set axis to y");
			} else if (key == SDLK_z) {
				_axis |= voxedit::Axis::Z;
				Log::debug("Set axis to z");
			}
			_lastModePress = core::App::getInstance()->timeProvider()->tickTime();
		}
	}

	return ui::Window::OnEvent(ev);
}

void VoxEditWindow::OnDie() {
	Super::OnDie();
	requestQuit();
}

void VoxEditWindow::copy() {
	_scene->copy();
}

void VoxEditWindow::paste() {
	_scene->paste();
}

void VoxEditWindow::cut() {
	_scene->cut();
}

void VoxEditWindow::undo() {
	_scene->undo();
}

void VoxEditWindow::redo() {
	_scene->redo();
}

void VoxEditWindow::quit() {
	if (_scene->isDirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and quit?",
				ui::Window::PopupType::YesNo, "unsaved_changes_quit");
		return;
	}
	Close();
}

bool VoxEditWindow::save(std::string_view file) {
	if (file.empty()) {
		const std::string& f = _voxedit->saveDialog("vox,qbt,qb");
		if (f.empty()) {
			return false;
		}
		if (!_scene->saveModel(f)) {
			Log::warn("Failed to save the model");
			return false;
		}
		Log::info("Saved the model to %s", f.data());
		return true;
	}

	if (!_scene->saveModel(file)) {
		Log::warn("Failed to save the model");
		return false;
	}
	Log::info("Saved the model to %s", file.data());
	return true;
}

bool VoxEditWindow::voxelize(std::string_view file) {
	std::string f;
	if (file.empty()) {
		f = _voxedit->openDialog("vox,qbt,qb");
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

bool VoxEditWindow::exportFile(std::string_view file) {
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

void VoxEditWindow::resetcamera() {
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

bool VoxEditWindow::load(std::string_view file) {
	std::string f;
	if (file.empty()) {
		f = _voxedit->openDialog("vox,qbt,qb");
		if (f.empty()) {
			return false;
		}
		file = f;
	}

	if (!_scene->isDirty()) {
		if (_scene->loadModel(file)) {
			resetcamera();
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

void VoxEditWindow::select(const glm::ivec3& pos) {
	_scene->select(pos);
}

bool VoxEditWindow::createNew(bool force) {
	if (!force && _scene->isDirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and close?",
				ui::Window::PopupType::YesNo, "unsaved_changes_new");
	}
	if (_scene->newModel(force)) {
		resetcamera();
		return true;
	}
	return false;
}

}
