#include "VoxEditWindow.h"
#include "LSystemWindow.h"
#include "NoiseWindow.h"
#include "TreeWindow.h"
#include "editorscene/EditorScene.h"
#include "palette/PaletteWidget.h"
#include "io/Filesystem.h"
#include "core/Array.h"
#include "../VoxEdit.h"
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/importerdesc.h>
#include <set>

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
	{TBIDC("actionselectedge"),			SelectType::Edge},
	{TBIDC("actionselectaabb"),			SelectType::AABB}
};

static const struct {
	tb::TBID id;
	Shape shape;
} shapes[] = {
	{TBIDC("shapecone"),		Shape::Cone},
	{TBIDC("shapesingle"),		Shape::Single},
	{TBIDC("shapesphere"),		Shape::Sphere},
	{TBIDC("shapecircle"),		Shape::Circle},
	{TBIDC("shapedome"),		Shape::Dome},
	{TBIDC("shapetorus"),		Shape::Torus},
	{TBIDC("shapeplane"),		Shape::Plane}
};

static const struct {
	const char *name;
	const char *id;
	tb::TBID tbid;
	voxel::TreeType type;
} treeTypes[] = {
	{"Pine",				"tree_pine",				TBIDC("tree_pine"),					voxel::TreeType::Pine},
	{"Dome",				"tree_dome",				TBIDC("tree_dome"),					voxel::TreeType::Dome},
	{"Dome Hanging",		"tree_dome2",				TBIDC("tree_dome2"),				voxel::TreeType::DomeHangingLeaves},
	{"Cone",				"tree_cone",				TBIDC("tree_cone"),					voxel::TreeType::Cone},
	{"Fir",					"tree_fir",					TBIDC("tree_fir"),					voxel::TreeType::Fir},
	{"Ellipsis2",			"tree_ellipsis2",			TBIDC("tree_ellipsis2"),			voxel::TreeType::BranchesEllipsis},
	{"Ellipsis",			"tree_ellipsis",			TBIDC("tree_ellipsis"),				voxel::TreeType::Ellipsis},
	{"Cube",				"tree_cube",				TBIDC("tree_cube"),					voxel::TreeType::Cube},
	{"Cube Sides",			"tree_cube2",				TBIDC("tree_cube2"),				voxel::TreeType::CubeSideCubes},
	{"Palm",				"tree_palm",				TBIDC("tree_palm"),					voxel::TreeType::Palm},
	{"SpaceColonization",	"tree_spacecolonization",	TBIDC("tree_spacecolonization"),	voxel::TreeType::SpaceColonization}
};
static_assert(lengthof(treeTypes) == (int)voxel::TreeType::Max, "Missing support for tree types in the ui");

static const struct {
	const char *name;
	const char *id;
	tb::TBID tbid;
	voxel::PlantType type;
} plantTypes[] = {
	{"Flower",		"plant_flower",		TBIDC("plant_flower"),		voxel::PlantType::Flower},
	{"Grass",		"plant_grass",		TBIDC("plant_grass"),		voxel::PlantType::Grass},
	{"Mushroom",	"plant_mushroom",	TBIDC("plant_mushroom"),	voxel::PlantType::Mushroom}
};
static_assert(lengthof(plantTypes) == (int)voxel::PlantType::MaxPlantTypes, "Missing support for plant types in the ui");

static const struct {
	const char *name;
	const char *id;
	tb::TBID tbid;
	voxel::BuildingType type;
} buildingTypes[] = {
	{"Tower",		"building_tower",	TBIDC("building_tower"),	voxel::BuildingType::Tower},
	{"House",		"building_house",	TBIDC("building_house"),	voxel::BuildingType::House}
};
static_assert(lengthof(buildingTypes) == (int)voxel::BuildingType::Max, "Missing support for building types in the ui");

VoxEditWindow::VoxEditWindow(VoxEdit* tool) :
		Super(tool), _scene(nullptr), _voxedit(tool), _paletteWidget(nullptr) {
	SetSettings(tb::WINDOW_SETTINGS_CAN_ACTIVATE);
	for (int i = 0; i < lengthof(treeTypes); ++i) {
		addStringItem(_treeItems, treeTypes[i].name, treeTypes[i].id);
	}
	addStringItem(_fileItems, "New");
	addStringItem(_fileItems, "Load");
	addStringItem(_fileItems, "Save");
	addStringItem(_fileItems, "Import");
	addStringItem(_fileItems, "Prefab");
	addStringItem(_fileItems, "Export");
	addStringItem(_fileItems, "Heightmap");
	addStringItem(_fileItems, "Quit");

	addStringItem(_plantItems, "Cactus", "cactus");
	for (int i = 0; i < lengthof(plantTypes); ++i) {
		addStringItem(_plantItems, plantTypes[i].name, plantTypes[i].id);
	}

	for (int i = 0; i < lengthof(buildingTypes); ++i) {
		addStringItem(_buildingItems, buildingTypes[i].name, buildingTypes[i].id);
	}

	addStringItem(_structureItems, "Trees")->sub_source = &_treeItems;
	addStringItem(_structureItems, "Plants", "plants")->sub_source = &_plantItems;
	addStringItem(_structureItems, "Clouds", "clouds");
	addStringItem(_structureItems, "Buildings", "buildings")->sub_source = &_buildingItems;
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
	const int8_t index = (uint8_t)_paletteWidget->GetValue();
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
	_scene->setVoxel(voxel);
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

	_mirrorX = getWidgetByType<tb::TBRadioButton>("mirrorx");
	_mirrorY = getWidgetByType<tb::TBRadioButton>("mirrory");
	_mirrorZ = getWidgetByType<tb::TBRadioButton>("mirrorz");

	_showAABB = getWidgetByType<tb::TBCheckBox>("optionshowaabb");
	_showGrid = getWidgetByType<tb::TBCheckBox>("optionshowgrid");
	_showAxis = getWidgetByType<tb::TBCheckBox>("optionshowaxis");
	_showLockAxis = getWidgetByType<tb::TBCheckBox>("optionshowlockaxis");
	_freeLook = getWidgetByType<tb::TBCheckBox>("optionfreelook");
	if (_showAABB == nullptr || _showGrid == nullptr || _showLockAxis == nullptr || _showAxis == nullptr || _freeLook == nullptr) {
		Log::error("Could not load all required widgets");
		return false;
	}

	_showAABB->SetValue(_scene->renderAABB() ? 1 : 0);
	_showGrid->SetValue(_scene->renderGrid() ? 1 : 0);
	_showAxis->SetValue(_scene->renderAxis() ? 1 : 0);
	_showLockAxis->SetValue(_scene->renderLockAxis() ? 1 : 0);
	_freeLook->SetValue(_scene->camera().rotationType() == video::CameraRotationType::Eye ? 1 : 0);

	Assimp::Exporter exporter;
	const size_t exporterNum = exporter.GetExportFormatCount();
	for (size_t i = 0; i < exporterNum; ++i) {
		const aiExportFormatDesc* desc = exporter.GetExportFormatDescription(i);
		_exportFilter.append(desc->fileExtension);
		if (i < exporterNum - 1) {
			_exportFilter.append(";");
		}
	}

	Assimp::Importer importer;
	const size_t importerNum = importer.GetImporterCount();
	std::set<std::string> importExtensions;
	for (size_t i = 0; i < importerNum; ++i) {
		const aiImporterDesc* desc = importer.GetImporterInfo(i);
		const char* ext = desc->mFileExtensions;
		const char* last = ext;
		do {
			if (ext[0] == '\0' || ext[0] == ' ') {
				importExtensions.insert(std::string(last, ext - last));
				last = ext;
				while (*last == ' ') {
					++last;
				}
			}
		} while (*ext++);
	}
	const int importerExtensionCount = importExtensions.size();
	int n = 0;
	for (auto i = importExtensions.begin(); i != importExtensions.end(); ++i, ++n) {
		_importFilter.append(*i);
		if (n < importerExtensionCount - 1) {
			_importFilter.append(";");
		}
	}
	Log::info("Supported import filters: %s", _importFilter.c_str());
	Log::info("Supported export filters: %s", _exportFilter.c_str());

	return true;
}

void VoxEditWindow::update() {
	_scene->update();
	_sceneTop->update();
	_sceneLeft->update();
	_sceneFront->update();
}

bool VoxEditWindow::isFocused() const {
	return tb::TBWidget::focused_widget == _scene
			|| tb::TBWidget::focused_widget == _sceneTop
			|| tb::TBWidget::focused_widget == _sceneLeft
			|| tb::TBWidget::focused_widget == _sceneFront;
}

bool VoxEditWindow::isHovered() const {
	return tb::TBWidget::hovered_widget == _scene
			|| tb::TBWidget::hovered_widget == _sceneTop
			|| tb::TBWidget::hovered_widget == _sceneLeft
			|| tb::TBWidget::hovered_widget == _sceneFront;
}

void VoxEditWindow::setCursorPosition(int x, int y, int z, bool relative) {
	if (relative) {
		glm::ivec3 p = _scene->cursorPosition();
		p.x += x;
		p.y += y;
		p.z += z;
		_scene->setCursorPosition(p, true);
	} else {
		_scene->setCursorPosition(glm::ivec3(x, y, z), true);
	}
}

void VoxEditWindow::place() {
	_scene->place();
}

void VoxEditWindow::remove() {
	_scene->remove();
}

void VoxEditWindow::rotate(int x, int y, int z) {
	Log::debug("execute rotate by %i:%i:%i", x, y, z);
	_scene->rotate(x, y, z);
}

bool VoxEditWindow::resample(int factor) {
	Log::debug("execute resample with factor %i", factor);
	return _scene->resample(factor);
}

void VoxEditWindow::scaleCursor(float x, float y, float z) {
	Log::debug("execute cursor scale by %f:%f:%f", x, y, z);
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
		_axis = math::Axis::None;
		return;
	}

	if (_modeNumberBuf[0] != '\0') {
		if (_mode == ModifierMode::Scale) {
			const float value = core::string::toFloat(_modeNumberBuf);
			glm::vec3 values(1.0f, 1.0f, 1.0f);
			if ((_axis & math::Axis::X) != math::Axis::None) {
				values.x = value;
			}
			if ((_axis & math::Axis::Y) != math::Axis::None) {
				values.y = value;
			}
			if ((_axis & math::Axis::Z) != math::Axis::None) {
				values.z = value;
			}
			scaleCursor(values.x, values.y, values.z);
		} else {
			const int value = core::string::toInt(_modeNumberBuf);
			glm::ivec3 values(0, 0, 0);
			if ((_axis & math::Axis::X) != math::Axis::None) {
				values.x = value;
			}
			if ((_axis & math::Axis::Y) != math::Axis::None) {
				values.y = value;
			}
			if ((_axis & math::Axis::Z) != math::Axis::None) {
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
		const math::Axis locked = _scene->lockedAxis();
#define VOXEDIT_LOCK(axis) if ((_axis & axis) != math::Axis::None) { _scene->setLockedAxis(axis, (locked & axis) != math::Axis::None); _lockedDirty = true; }
		VOXEDIT_LOCK(math::Axis::X)
		VOXEDIT_LOCK(math::Axis::Y)
		VOXEDIT_LOCK(math::Axis::Z)
#undef VOXEDIT_LOCK
	} else if (_mode == ModifierMode::Mirror) {
#define VOXEDIT_MIRROR(axis) if (_axis == axis) { _scene->setMirrorAxis(axis, _scene->referencePosition()); _mirrorDirty = true; }
		VOXEDIT_MIRROR(math::Axis::X)
		VOXEDIT_MIRROR(math::Axis::Y)
		VOXEDIT_MIRROR(math::Axis::Z)
#undef VOXEDIT_MIRROR
	}

	_modeNumberBuf[0] = '\0';
	_lastModePress = -1l;
	_axis = math::Axis::None;
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

void VoxEditWindow::setReferencePosition(int x, int y, int z) {
	_scene->setReferencePosition(glm::ivec3(x, y, z));
}

void VoxEditWindow::setreferencepositiontocursor() {
	_scene->setReferencePosition(_scene->cursorPosition());
}

void VoxEditWindow::unselectall() {
	_scene->unselectAll();
}

void VoxEditWindow::bezier(const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control) {
	_scene->bezier(start, end, control);
}

void VoxEditWindow::rotatemode() {
	_mode = ModifierMode::Rotate;
	_axis = math::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::scalemode() {
	_mode = ModifierMode::Scale;
	_axis = math::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::movemode() {
	_mode = ModifierMode::Move;
	_axis = math::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::togglelockaxis() {
	_mode = ModifierMode::Lock;
	_axis = math::Axis::None;
	_modeNumberBuf[0] = '\0';
}

void VoxEditWindow::togglemirroraxis() {
	_mode = ModifierMode::Mirror;
	_axis = math::Axis::None;
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

static inline bool isAny(const tb::TBWidgetEvent& ev, const tb::TBID& id) {
	return ev.target->GetID() == id || ev.ref_id == id;
}

bool VoxEditWindow::handleEvent(const tb::TBWidgetEvent &ev) {
	if (isAny(ev, TBIDC("resetcamera"))) {
		_scene->resetCamera();
		_sceneFront->resetCamera();
		_sceneLeft->resetCamera();
		_sceneTop->resetCamera();
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
	} else if (isAny(ev, TBIDC("fill"))) {
		const glm::ivec3& pos = _scene->cursorPosition();
		fill(pos.x, pos.y, pos.z);
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
	} else if (isAny(ev, TBIDC("import"))) {
		importMesh("");
		return true;
	} else if (isAny(ev, TBIDC("prefab"))) {
		prefab("");
		return true;
	} else if (isAny(ev, TBIDC("spacecolonization"))) {
		_scene->spaceColonization();
		return true;
	} else if (isAny(ev, TBIDC("heightmap"))) {
		importHeightmap("");
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
	} else if (isAny(ev, TBIDC("resample"))) {
		resample(2);
		return true;
	} else if (isAny(ev, TBIDC("menu_structure"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("structure_popup"))) {
			menu->Show(&_structureItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (isAny(ev, TBIDC("menu_tree"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("tree_popup"))) {
			menu->Show(&_treeItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (isAny(ev, TBIDC("menu_file"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("menu_file_window"))) {
			menu->Show(&_fileItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (isAny(ev, TBIDC("dialog_lsystem"))) {
		new LSystemWindow(this, _scene);
		return true;
	} else if (isAny(ev, TBIDC("dialog_noise"))) {
		new NoiseWindow(this, _scene);
		return true;
	} else if (isAny(ev, TBIDC("optionshowgrid"))) {
		_scene->setRenderGrid(ev.target->GetValue() == 1);
		return true;
	} else if (isAny(ev, TBIDC("optionshowaxis"))) {
		_scene->setRenderAxis(ev.target->GetValue() == 1);
		return true;
	} else if (isAny(ev, TBIDC("optionshowlockaxis"))) {
		_scene->setRenderLockAxis(ev.target->GetValue() == 1);
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

	for (int i = 0; i < lengthof(actions); ++i) {
		if (isAny(ev, actions[i].id)) {
			_scene->setAction(actions[i].action);
			return true;
		}
	}
	for (int i = 0; i < lengthof(selectionmodes); ++i) {
		if (isAny(ev, selectionmodes[i].id)) {
			_scene->setSelectionType(selectionmodes[i].type);
			setAction(Action::SelectVoxels);
			return true;
		}
	}
	for (int i = 0; i < lengthof(shapes); ++i) {
		if (isAny(ev, shapes[i].id)) {
			_scene->setCursorShape(shapes[i].shape);
			return true;
		}
	}
	for (int i = 0; i < lengthof(treeTypes); ++i) {
		if (isAny(ev, treeTypes[i].tbid)) {
			new TreeWindow(this, _scene, treeTypes[i].type);
			return true;
		}
	}
	for (int i = 0; i < lengthof(buildingTypes); ++i) {
		if (isAny(ev, buildingTypes[i].tbid)) {
			voxel::BuildingContext ctx;
			if (buildingTypes[i].type == voxel::BuildingType::Tower) {
				ctx.floors = 3;
			}
			_scene->createBuilding(buildingTypes[i].type, ctx);
			return true;
		}
	}
	for (int i = 0; i < lengthof(plantTypes); ++i) {
		if (isAny(ev, plantTypes[i].tbid)) {
			_scene->createPlant(plantTypes[i].type);
			return true;
		}
	}
	if (isAny(ev, TBIDC("clouds"))) {
		_scene->createCloud();
		return true;
	} else if (isAny(ev, TBIDC("cactus"))) {
		_scene->createCactus();
		return true;
	}

#ifdef TB_RUNTIME_DEBUG_INFO
	Log::debug("Unknown event %s - %s", ev.target->GetID().debug_string.CStr(), ev.ref_id.debug_string.CStr());
#endif

	return false;
}

void VoxEditWindow::setSelectionType(SelectType type) {
	for (int i = 0; i < lengthof(selectionmodes); ++i) {
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
	for (int i = 0; i < lengthof(actions); ++i) {
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

void VoxEditWindow::extend(const glm::ivec3& size) {
	_scene->extend(size);
}

void VoxEditWindow::scaleHalf() {
	_scene->scaleHalf();
}

void VoxEditWindow::fill() {
	const glm::ivec3& pos = _scene->referencePosition();
	fill(pos.x, pos.y, pos.z);
}

void VoxEditWindow::fill(int x, int y, int z) {
	_scene->fill(x, y, z);
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
		_scene->setLockedAxis(math::Axis::X, ev.target->GetValue() != 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("locky")) {
		_scene->setLockedAxis(math::Axis::Y, ev.target->GetValue() != 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("lockz")) {
		_scene->setLockedAxis(math::Axis::Z, ev.target->GetValue() != 1);
		return true;
	} else if (ev.target->GetID() == TBIDC("mirrorx")) {
		_scene->setMirrorAxis(math::Axis::X, _scene->referencePosition());
		return true;
	} else if (ev.target->GetID() == TBIDC("mirrory")) {
		_scene->setMirrorAxis(math::Axis::Y, _scene->referencePosition());
		return true;
	} else if (ev.target->GetID() == TBIDC("mirrorz")) {
		_scene->setMirrorAxis(math::Axis::Z, _scene->referencePosition());
		return true;
	} else if (ev.target->GetID() == TBIDC("mirrornone")) {
		_scene->setMirrorAxis(math::Axis::None, _scene->referencePosition());
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

	if (_lastModePress > 0l && core::App::getInstance()->timeProvider()->tickMillis() - _lastModePress > 1500l) {
		executeMode();
	}

	if (_paletteWidget->isDirty()) {
		const int8_t index = (uint8_t)_paletteWidget->GetValue();
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
		_scene->setVoxel(voxel);
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
		_undoButton->SetState(tb::WIDGET_STATE_DISABLED, !_scene->canUndo());
	}
	if (_redoButton != nullptr) {
		_redoButton->SetState(tb::WIDGET_STATE_DISABLED, !_scene->canRedo());
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
		const math::Axis axis = _scene->lockedAxis();
		if (_lockedX != nullptr) {
			_lockedX->SetValue((axis & math::Axis::X) != math::Axis::None);
		}
		if (_lockedY != nullptr) {
			_lockedY->SetValue((axis & math::Axis::Y) != math::Axis::None);
		}
		if (_lockedZ != nullptr) {
			_lockedZ->SetValue((axis & math::Axis::Z) != math::Axis::None);
		}
	}

	if (_mirrorDirty) {
		_mirrorDirty = false;
		const math::Axis axis = _scene->mirrorAxis();
		if (_mirrorX != nullptr) {
			_mirrorX->SetValue(axis == math::Axis::X);
		}
		if (_mirrorY != nullptr) {
			_mirrorY->SetValue(axis == math::Axis::Y);
		}
		if (_mirrorZ != nullptr) {
			_mirrorZ->SetValue(axis == math::Axis::Z);
		}
	}

	for (int i = 0; i < lengthof(actions); ++i) {
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
		if (_axis != math::Axis::None) {
			if (isValidNumberKey(key)) {
				int l = SDL_strlen(_modeNumberBuf);
				if (l < MODENUMBERBUFSIZE - 1) {
					_modeNumberBuf[l++] = (uint8_t)key;
					_modeNumberBuf[l] = '\0';
					_lastModePress = core::App::getInstance()->timeProvider()->tickMillis();
				}
			} else if (ev.special_key == tb::TB_KEY_ENTER) {
				executeMode();
			}
		} else if (_mode != ModifierMode::None) {
			if (key == SDLK_x) {
				Log::debug("Set axis to x");
				_axis |= math::Axis::X;
			} else if (key == SDLK_y) {
				_axis |= math::Axis::Y;
				Log::debug("Set axis to y");
			} else if (key == SDLK_z) {
				_axis |= math::Axis::Z;
				Log::debug("Set axis to z");
			}
			_lastModePress = core::App::getInstance()->timeProvider()->tickMillis();
		}
	}

	return Super::OnEvent(ev);
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
				ui::turbobadger::Window::PopupType::YesNo, "unsaved_changes_quit");
		return;
	}
	Close();
}

bool VoxEditWindow::importHeightmap(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string& file) { importHeightmap(file); }, "png");
		return true;
	}

	if (!_scene->importHeightmap(file)) {
		return false;
	}
	return true;
}

bool VoxEditWindow::save(const std::string& file) {
	if (file.empty()) {
		getApp()->saveDialog([this] (const std::string& file) {save(file); }, "vox,qbt,qb");
		return true;
	}
	if (!_scene->saveModel(file)) {
		Log::warn("Failed to save the model");
		return false;
	}
	Log::info("Saved the model to %s", file.c_str());
	return true;
}

bool VoxEditWindow::importMesh(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string& file) {importMesh(file);}, _importFilter);
		return true;
	}
	if (!_scene->isDirty()) {
		const video::MeshPtr& mesh = _voxedit->meshPool()->getMesh(file, false);
		return _scene->voxelizeModel(mesh);
	}

	_voxelizeFile = file;
	popup("Unsaved Modifications",
			"There are unsaved modifications.\nDo you wish to discard them and start the voxelize process?",
			ui::turbobadger::Window::PopupType::YesNo, "unsaved_changes_voxelize");
	return true;
}

bool VoxEditWindow::exportFile(const std::string& file) {
	if (_scene->isEmpty()) {
		return false;
	}

	if (file.empty()) {
		if (_exportFilter.empty()) {
			return false;
		}
		getApp()->saveDialog([this] (const std::string& file) { exportFile(file); }, _exportFilter);
		return true;
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

bool VoxEditWindow::prefab(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string& file) { prefab(file); }, "vox,qbt,qb");
		return true;
	}

	return _scene->prefab(file);
}

bool VoxEditWindow::load(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string& file) { load(file); }, "vox,qbt,qb");
		return true;
	}

	if (!_scene->isDirty()) {
		if (_scene->loadModel(file)) {
			resetcamera();
			return true;
		}
		return false;
	}

	_loadFile = file;
	popup("Unsaved Modifications",
			"There are unsaved modifications.\nDo you wish to discard them and load?",
			ui::turbobadger::Window::PopupType::YesNo, "unsaved_changes_load");
	return false;
}

void VoxEditWindow::selectCursor() {
	const glm::ivec3& pos = _scene->cursorPosition();
	select(pos);
}

void VoxEditWindow::select(const glm::ivec3& pos) {
	_scene->select(pos);
}

bool VoxEditWindow::createNew(bool force) {
	if (!force && _scene->isDirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and close?",
				ui::turbobadger::Window::PopupType::YesNo, "unsaved_changes_new");
	} else if (_scene->newModel(force)) {
		return true;
	}
	return false;
}

}
