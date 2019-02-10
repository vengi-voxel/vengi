/**
 * @file
 */

#include "VoxEditWindow.h"
#include "NoiseWindow.h"
#include "TreeWindow.h"
#include "palette/PaletteWidget.h"
#include "io/Filesystem.h"
#include "video/WindowedApp.h"
#include "editorscene/ViewportSingleton.h"
#include "editorscene/Viewport.h"
#include "../VoxEdit.h"
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/importerdesc.h>
#include <set>

namespace voxedit {

static inline ViewportSingleton& vps() {
	return ViewportSingleton::getInstance();
}

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
	setSettings(tb::WINDOW_SETTINGS_CAN_ACTIVATE);
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
	_scene = getWidgetByType<Viewport>("editorscene");
	if (_scene == nullptr) {
		Log::error("Failed to init the main window: Could not get the editor scene node with id 'editorscene'");
		return false;
	}

	_paletteWidget = getWidgetByType<PaletteWidget>("palettecontainer");
	if (_paletteWidget == nullptr) {
		Log::error("Failed to init the main window: Could not get the editor scene node with id 'palettecontainer'");
		return false;
	}
	const int8_t index = (uint8_t)_paletteWidget->getValue();
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
	vps().setCursorVoxel(voxel);
	_paletteWidget->markAsClean();

	_sceneTop = getWidgetByType<Viewport>("editorscenetop");
	_sceneLeft = getWidgetByType<Viewport>("editorsceneleft");
	_sceneFront = getWidgetByType<Viewport>("editorscenefront");

	_fourViewAvailable = _sceneTop != nullptr && _sceneLeft != nullptr && _sceneFront != nullptr;

	tb::TBWidget* toggleViewPort = getWidget("toggleviewport");
	if (toggleViewPort != nullptr) {
		toggleViewPort->setState(tb::WIDGET_STATE_DISABLED, !_fourViewAvailable);
		const int value = toggleViewPort->getValue();
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

	_mirrorNone = getWidgetByType<tb::TBRadioButton>("mirrornone");
	_mirrorX = getWidgetByType<tb::TBRadioButton>("mirrorx");
	_mirrorY = getWidgetByType<tb::TBRadioButton>("mirrory");
	_mirrorZ = getWidgetByType<tb::TBRadioButton>("mirrorz");

	_placeModifier = getWidgetByType<tb::TBRadioButton>("actionplace");
	_deleteModifier = getWidgetByType<tb::TBRadioButton>("actiondelete");
	_overrideModifier = getWidgetByType<tb::TBRadioButton>("actionoverride");

	_showAABB = getWidgetByType<tb::TBCheckBox>("optionshowaabb");
	_showGrid = getWidgetByType<tb::TBCheckBox>("optionshowgrid");
	_showAxis = getWidgetByType<tb::TBCheckBox>("optionshowaxis");
	_showLockAxis = getWidgetByType<tb::TBCheckBox>("optionshowlockaxis");
	_renderShadow = getWidgetByType<tb::TBCheckBox>("optionrendershadow");
	if (_showAABB == nullptr || _showGrid == nullptr || _showLockAxis == nullptr
	 || _showAxis == nullptr || _renderShadow == nullptr) {
		Log::error("Could not load all required widgets");
		return false;
	}

	const render::GridRenderer& gridRenderer = vps().gridRenderer();
	_showAABB->setValue(gridRenderer.renderAABB() ? 1 : 0);
	_showGrid->setValue(gridRenderer.renderGrid() ? 1 : 0);
	_showAxis->setValue(vps().renderAxis() ? 1 : 0);
	_showLockAxis->setValue(vps().renderLockAxis() ? 1 : 0);
	_renderShadow->setValue(vps().renderShadow() ? 1 : 0);

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
		glm::ivec3 p = vps().cursorPosition();
		p.x += x;
		p.y += y;
		p.z += z;
		vps().setCursorPosition(p, true);
	} else {
		vps().setCursorPosition(glm::ivec3(x, y, z), true);
	}
}

void VoxEditWindow::rotate(int x, int y, int z) {
	Log::debug("execute rotate by %i:%i:%i", x, y, z);
	vps().rotate(x, y, z);
}

void VoxEditWindow::move(int x, int y, int z) {
	Log::debug("execute move by %i:%i:%i", x, y, z);
	vps().move(x, y, z);
}

void VoxEditWindow::toggleviewport() {
	bool vis = false;
	if (_sceneTop != nullptr) {
		vis = _sceneTop->getVisibilityCombined();
	}
	if (!vis && _sceneLeft != nullptr) {
		vis = _sceneLeft->getVisibilityCombined();
	}
	if (!vis && _sceneFront != nullptr) {
		vis = _sceneFront->getVisibilityCombined();
	}

	setQuadViewport(!vis);
}

void VoxEditWindow::setReferencePosition(int x, int y, int z) {
	vps().setReferencePosition(glm::ivec3(x, y, z));
}

void VoxEditWindow::setreferencepositiontocursor() {
	vps().setReferencePosition(vps().cursorPosition());
}

void VoxEditWindow::setQuadViewport(bool active) {
	const tb::WIDGET_VISIBILITY vis = active ? tb::WIDGET_VISIBILITY_VISIBLE : tb::WIDGET_VISIBILITY_GONE;
	if (_sceneTop != nullptr) {
		_sceneTop->setVisibility(vis);
	}
	if (_sceneLeft != nullptr) {
		_sceneLeft->setVisibility(vis);
	}
	if (_sceneFront != nullptr) {
		_sceneFront->setVisibility(vis);
	}
	tb::TBWidget* toggleViewPort = getWidget("toggleviewport");
	if (toggleViewPort != nullptr) {
		toggleViewPort->setValue(active ? 1 : 0);
	}
}

bool VoxEditWindow::handleEvent(const tb::TBWidgetEvent &ev) {
	if (ev.isAny(TBIDC("resetcamera"))) {
		_scene->resetCamera();
		_sceneFront->resetCamera();
		_sceneLeft->resetCamera();
		_sceneTop->resetCamera();
		return true;
	} else if (ev.isAny(TBIDC("quit"))) {
		quit();
		return true;
	} else if (ev.isAny(TBIDC("crop"))) {
		vps().crop();
		return true;
	} else if (ev.isAny(TBIDC("extend"))) {
		extend();
		return true;
	} else if (ev.isAny(TBIDC("new"))) {
		createNew(false);
		return true;
	} else if (ev.isAny(TBIDC("load"))) {
		load("");
		return true;
	} else if (ev.isAny(TBIDC("export"))) {
		exportFile("");
		return true;
	} else if (ev.isAny(TBIDC("import"))) {
		importMesh("");
		return true;
	} else if (ev.isAny(TBIDC("prefab"))) {
		prefab("");
		return true;
	} else if (ev.isAny(TBIDC("spacecolonization"))) {
		vps().spaceColonization();
		return true;
	} else if (ev.isAny(TBIDC("heightmap"))) {
		importHeightmap("");
		return true;
	} else if (ev.isAny(TBIDC("save"))) {
		save("");
		return true;
	} else if (ev.isAny(TBIDC("redo"))) {
		redo();
		return true;
	} else if (ev.isAny(TBIDC("undo"))) {
		undo();
		return true;
	} else if (ev.isAny(TBIDC("rotatex"))) {
		rotatex();
		return true;
	} else if (ev.isAny(TBIDC("rotatey"))) {
		rotatey();
		return true;
	} else if (ev.isAny(TBIDC("rotatez"))) {
		rotatez();
		return true;
	} else if (ev.isAny(TBIDC("menu_structure"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("structure_popup"))) {
			menu->show(&_structureItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (ev.isAny(TBIDC("menu_tree"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("tree_popup"))) {
			menu->show(&_treeItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (ev.isAny(TBIDC("menu_file"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("menu_file_window"))) {
			menu->show(&_fileItems, tb::TBPopupAlignment());
		}
		return true;
	} else if (ev.isAny(TBIDC("dialog_noise"))) {
		new NoiseWindow(this);
		return true;
	} else if (ev.isAny(TBIDC("optionshowgrid"))) {
		vps().gridRenderer().setRenderGrid(ev.target->getValue() == 1);
		return true;
	} else if (ev.isAny(TBIDC("optionshowaxis"))) {
		vps().setRenderAxis(ev.target->getValue() == 1);
		return true;
	} else if (ev.isAny(TBIDC("optionshowlockaxis"))) {
		vps().setRenderLockAxis(ev.target->getValue() == 1);
		return true;
	} else if (ev.isAny(TBIDC("optionshowaabb"))) {
		vps().gridRenderer().setRenderAABB(ev.target->getValue() == 1);
		return true;
	} else if (ev.isAny(TBIDC("optionrendershadow"))) {
		vps().setRenderShadow(ev.target->getValue() == 1);
		return true;
	}
	return false;
}

bool VoxEditWindow::handleClickEvent(const tb::TBWidgetEvent &ev) {
	tb::TBWidget *widget = ev.target;
	const tb::TBID &id = widget->getID();
	if (id == TBIDC("unsaved_changes_new")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			_scene->newModel(true);
		}
		return true;
	} else if (id == TBIDC("unsaved_changes_quit")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			close();
		}
		return true;
	} else if (id == TBIDC("unsaved_changes_load")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			vps().load(_loadFile);
			resetcamera();
		}
		return true;
	} else if (id == TBIDC("unsaved_changes_voxelize")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			const video::MeshPtr& mesh = _voxedit->meshPool()->getMesh(_voxelizeFile, false);
			vps().voxelizeModel(mesh);
		}
		return true;
	}

	if (handleEvent(ev)) {
		return true;
	}

	for (int i = 0; i < lengthof(treeTypes); ++i) {
		if (ev.isAny(treeTypes[i].tbid)) {
			new TreeWindow(this, treeTypes[i].type);
			return true;
		}
	}
	for (int i = 0; i < lengthof(buildingTypes); ++i) {
		if (ev.isAny(buildingTypes[i].tbid)) {
			voxel::BuildingContext ctx;
			if (buildingTypes[i].type == voxel::BuildingType::Tower) {
				ctx.floors = 3;
			}
			vps().createBuilding(buildingTypes[i].type, ctx);
			return true;
		}
	}
	for (int i = 0; i < lengthof(plantTypes); ++i) {
		if (ev.isAny(plantTypes[i].tbid)) {
			vps().createPlant(plantTypes[i].type);
			return true;
		}
	}
	if (ev.isAny(TBIDC("clouds"))) {
		vps().createCloud();
		return true;
	} else if (ev.isAny(TBIDC("cactus"))) {
		vps().createCactus();
		return true;
	}

	return false;
}

void VoxEditWindow::extend(const glm::ivec3& size) {
	vps().extend(size);
}

bool VoxEditWindow::handleChangeEvent(const tb::TBWidgetEvent &ev) {
	tb::TBWidget *widget = ev.target;
	const tb::TBID &id = widget->getID();
	if (id == TBIDC("camrottype")) {
		tb::TBWidget *parent = widget->getParent();
		if (Viewport *viewport = parent->safeCastTo<Viewport>()) {
			const int value = widget->getValue();
			const video::CameraRotationType type = value == 1 ?
					video::CameraRotationType::Eye :
					video::CameraRotationType::Target;
			viewport->camera().setRotationType(type);
			return true;
		}
	} else if (id == TBIDC("cammode")) {
		tb::TBWidget *parent = widget->getParent();
		if (Viewport *viewport = parent->safeCastTo<Viewport>()) {
			const int value = widget->getValue();
			video::PolygonMode mode = video::PolygonMode::Solid;
			if (value == 1) {
				mode = video::PolygonMode::Points;
			} else if (value == 2) {
				mode = video::PolygonMode::WireFrame;
			}
			viewport->camera().setPolygonMode(mode);
			return true;
		}
	} else if (id == TBIDC("toggleviewport")) {
		const int value = widget->getValue();
		setQuadViewport(value == 1);
		return true;
	} else if (id == TBIDC("lockx")) {
		vps().setLockedAxis(math::Axis::X, widget->getValue() != 1);
		return true;
	} else if (id == TBIDC("locky")) {
		vps().setLockedAxis(math::Axis::Y, widget->getValue() != 1);
		return true;
	} else if (id == TBIDC("lockz")) {
		vps().setLockedAxis(math::Axis::Z, widget->getValue() != 1);
		return true;
	} else if (id == TBIDC("mirrorx")) {
		vps().setMirrorAxis(math::Axis::X, vps().referencePosition());
		return true;
	} else if (id == TBIDC("mirrory")) {
		vps().setMirrorAxis(math::Axis::Y, vps().referencePosition());
		return true;
	} else if (id == TBIDC("mirrorz")) {
		vps().setMirrorAxis(math::Axis::Z, vps().referencePosition());
		return true;
	} else if (id == TBIDC("mirrornone")) {
		vps().setMirrorAxis(math::Axis::None, vps().referencePosition());
		return true;
	} else if (id == TBIDC("cursorx")) {
		const tb::TBStr& str = widget->getText();
		if (str.isEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = vps().cursorPosition();
		pos.x = val;
		vps().setCursorPosition(pos, true);
		return true;
	} else if (id == TBIDC("cursory")) {
		const tb::TBStr& str = widget->getText();
		if (str.isEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = vps().cursorPosition();
		pos.y = val;
		vps().setCursorPosition(pos, true);
		return true;
	} else if (id == TBIDC("cursorz")) {
		const tb::TBStr& str = widget->getText();
		if (str.isEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = vps().cursorPosition();
		pos.z = val;
		vps().setCursorPosition(pos, true);
		return true;
	} else if (ev.isAny(TBIDC("actionplace") && widget->getValue() == 1)) {
		vps().setModifierType(ModifierType::Place);
		return true;
	} else if (ev.isAny(TBIDC("actiondelete")) && widget->getValue() == 1) {
		vps().setModifierType(ModifierType::Delete);
		return true;
	} else if (ev.isAny(TBIDC("actionupdate")) && widget->getValue() == 1) {
		vps().setModifierType(ModifierType::Update);
		return true;
	} else if (ev.isAny(TBIDC("actionoverride")) && widget->getValue() == 1) {
		vps().setModifierType(ModifierType::Place | ModifierType::Delete);
		return true;
	}

	return false;
}

void VoxEditWindow::onProcess() {
	Super::onProcess();

	if (_paletteWidget->isDirty()) {
		const int8_t index = (uint8_t)_paletteWidget->getValue();
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
		vps().setCursorVoxel(voxel);
		_paletteWidget->markAsClean();
	}
	const ModifierType modifierType = vps().modifierType();
	constexpr ModifierType overrideType = ModifierType::Delete | ModifierType::Place;
	if ((modifierType & overrideType) == overrideType) {
		if (_overrideModifier) {
			_overrideModifier->setValue(1);
		}
	} else if ((modifierType & ModifierType::Place) == ModifierType::Place) {
		if (_placeModifier) {
			_placeModifier->setValue(1);
		}
	} else if ((modifierType & ModifierType::Delete) == ModifierType::Delete) {
		if (_deleteModifier) {
			_deleteModifier->setValue(1);
		}
	}

	const bool empty = vps().empty();
	if (_exportButton != nullptr) {
		_exportButton->setState(tb::WIDGET_STATE_DISABLED, empty);
	}
	if (_saveButton != nullptr) {
		_saveButton->setState(tb::WIDGET_STATE_DISABLED, empty);
	}
	if (_undoButton != nullptr) {
		_undoButton->setState(tb::WIDGET_STATE_DISABLED, !vps().mementoHandler().canUndo());
	}
	if (_redoButton != nullptr) {
		_redoButton->setState(tb::WIDGET_STATE_DISABLED, !vps().mementoHandler().canRedo());
	}
	const glm::ivec3& pos = vps().cursorPosition();
	if (_lastCursorPos != pos) {
		_lastCursorPos = pos;
		char buf[64];
		if (_cursorX != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.x);
			if (SDL_strcmp(_cursorX->getText().c_str(), buf)) {
				_cursorX->setText(buf);
			}
		}
		if (_cursorY != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.y);
			if (SDL_strcmp(_cursorY->getText().c_str(), buf)) {
				_cursorY->setText(buf);
			}
		}
		if (_cursorZ != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.z);
			if (SDL_strcmp(_cursorZ->getText().c_str(), buf)) {
				_cursorZ->setText(buf);
			}
		}
	}

	const math::Axis lockedAxis = vps().lockedAxis();
	if (_lockedX != nullptr) {
		_lockedX->setValue((lockedAxis & math::Axis::X) != math::Axis::None);
	}
	if (_lockedY != nullptr) {
		_lockedY->setValue((lockedAxis & math::Axis::Y) != math::Axis::None);
	}
	if (_lockedZ != nullptr) {
		_lockedZ->setValue((lockedAxis & math::Axis::Z) != math::Axis::None);
	}

	const math::Axis mirrorAxis = vps().mirrorAxis();
	if (_mirrorNone != nullptr) {
		_mirrorNone->setValue(mirrorAxis == math::Axis::None);
	}
	if (_mirrorX != nullptr) {
		_mirrorX->setValue(mirrorAxis == math::Axis::X);
	}
	if (_mirrorY != nullptr) {
		_mirrorY->setValue(mirrorAxis == math::Axis::Y);
	}
	if (_mirrorZ != nullptr) {
		_mirrorZ->setValue(mirrorAxis == math::Axis::Z);
	}
}

bool VoxEditWindow::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CUSTOM) {
		if (handleEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_POINTER_DOWN) {
		if (Viewport* viewport = ev.target->safeCastTo<Viewport>()) {
			if (ev.button_type == tb::TB_LEFT || ev.button_type == tb::TB_RIGHT) {
				if (ev.button_type == tb::TB_RIGHT) {
					_modBeforeMouse = vps().modifierType();
					vps().setModifierType(ModifierType::Delete);
					vps().trace(viewport->camera(), true);
				}
				vps().aabbStart();
				return true;
			}
		}
	} else if (ev.type == tb::EVENT_TYPE_POINTER_UP) {
		if (Viewport* viewport = ev.target->safeCastTo<Viewport>()) {
			if (vps().aabbEnd()) {
				if (_modBeforeMouse != ModifierType::None) {
					vps().setModifierType(_modBeforeMouse);
					vps().trace(viewport->camera(), true);
					_modBeforeMouse = ModifierType::None;
				}
				return true;
			}
			return false;
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
			return true;
		} else if (ev.ref_id == TBIDC("redo")) {
			redo();
			return true;
		}
	}

	return Super::onEvent(ev);
}

void VoxEditWindow::onDie() {
	Super::onDie();
	requestQuit();
}

void VoxEditWindow::undo() {
	vps().undo();
}

void VoxEditWindow::redo() {
	vps().redo();
}

void VoxEditWindow::quit() {
	if (vps().dirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and quit?",
				ui::turbobadger::Window::PopupType::YesNo, "unsaved_changes_quit");
		return;
	}
	close();
}

bool VoxEditWindow::importHeightmap(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string& file) { importHeightmap(file); }, "png");
		return true;
	}

	return vps().importHeightmap(file);
}

bool VoxEditWindow::save(const std::string& file) {
	if (file.empty()) {
		getApp()->saveDialog([this] (const std::string& file) {save(file); }, "vox,qbt,qb");
		return true;
	}
	if (!vps().save(file)) {
		Log::warn("Failed to save the model");
		return false;
	}
	Log::info("Saved the model to %s", file.c_str());
	return true;
}

bool VoxEditWindow::saveScreenshot(const std::string& file) {
	if (file.empty()) {
		getApp()->saveDialog([this] (const std::string& file) {save(file); }, "png");
		return true;
	}
	if (!_scene->saveImage(file.c_str())) {
		Log::warn("Failed to save screenshot");
		return false;
	}
	Log::info("Screenshot created at '%s'", file.c_str());
	return true;
}

bool VoxEditWindow::importMesh(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string& file) {importMesh(file);}, _importFilter);
		return true;
	}
	if (!vps().dirty()) {
		const video::MeshPtr& mesh = _voxedit->meshPool()->getMesh(file, false);
		return vps().voxelizeModel(mesh);
	}

	_voxelizeFile = file;
	popup("Unsaved Modifications",
			"There are unsaved modifications.\nDo you wish to discard them and start the voxelize process?",
			ui::turbobadger::Window::PopupType::YesNo, "unsaved_changes_voxelize");
	return true;
}

bool VoxEditWindow::exportFile(const std::string& file) {
	if (vps().empty()) {
		return false;
	}

	if (file.empty()) {
		if (_exportFilter.empty()) {
			return false;
		}
		getApp()->saveDialog([this] (const std::string& file) { exportFile(file); }, _exportFilter);
		return true;
	}
	return vps().exportModel(file);
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

	return vps().prefab(file);
}

bool VoxEditWindow::load(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string& file) { load(file); }, "vox,qbt,qb");
		return true;
	}

	if (!vps().dirty()) {
		if (vps().load(file)) {
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

bool VoxEditWindow::createNew(bool force) {
	if (!force && vps().dirty()) {
		popup("Unsaved Modifications",
				"There are unsaved modifications.\nDo you wish to discard them and close?",
				ui::turbobadger::Window::PopupType::YesNo, "unsaved_changes_new");
	} else if (_scene->newModel(force)) {
		return true;
	}
	return false;
}

}
