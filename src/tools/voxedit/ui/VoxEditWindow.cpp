/**
 * @file
 */

#include "VoxEditWindow.h"
#include "NoiseWindow.h"
#include "TreeWindow.h"
#include "palette/PaletteWidget.h"
#include "palette/PaletteSelector.h"
#include "core/io/Filesystem.h"
#include "video/WindowedApp.h"
#include "core/Var.h"
#include "core/String.h"
#include "core/command/Command.h"
#include "editorscene/Viewport.h"
#include "settings/SceneSettingsWindow.h"
#include "layer/LayerWidget.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "../VoxEdit.h"
#include <set>

namespace voxedit {

static const char *SUPPORTED_VOXEL_FORMATS_LOAD = "vox,qbt,qb,vxm,binvox,cub";
static const char *SUPPORTED_VOXEL_FORMATS_SAVE = "vox,qbt,qb,cub";

static const struct {
	const char *name;
	const char *id;
	tb::TBID tbid;
	voxelgenerator::TreeType type;
} treeTypes[] = {
	{"Pine",				"tree_pine",				TBIDC("tree_pine"),					voxelgenerator::TreeType::Pine},
	{"Dome",				"tree_dome",				TBIDC("tree_dome"),					voxelgenerator::TreeType::Dome},
	{"Dome Hanging",		"tree_dome2",				TBIDC("tree_dome2"),				voxelgenerator::TreeType::DomeHangingLeaves},
	{"Cone",				"tree_cone",				TBIDC("tree_cone"),					voxelgenerator::TreeType::Cone},
	{"Fir",					"tree_fir",					TBIDC("tree_fir"),					voxelgenerator::TreeType::Fir},
	{"Ellipsis2",			"tree_ellipsis2",			TBIDC("tree_ellipsis2"),			voxelgenerator::TreeType::BranchesEllipsis},
	{"Ellipsis",			"tree_ellipsis",			TBIDC("tree_ellipsis"),				voxelgenerator::TreeType::Ellipsis},
	{"Cube",				"tree_cube",				TBIDC("tree_cube"),					voxelgenerator::TreeType::Cube},
	{"Cube Sides",			"tree_cube2",				TBIDC("tree_cube2"),				voxelgenerator::TreeType::CubeSideCubes},
	{"Palm",				"tree_palm",				TBIDC("tree_palm"),					voxelgenerator::TreeType::Palm},
	{"SpaceColonization",	"tree_spacecolonization",	TBIDC("tree_spacecolonization"),	voxelgenerator::TreeType::SpaceColonization}
};
static_assert(lengthof(treeTypes) == (int)voxelgenerator::TreeType::Max, "Missing support for tree types in the ui");

VoxEditWindow::VoxEditWindow(VoxEdit* tool) :
		Super(tool) {
	setSettings(tb::WINDOW_SETTINGS_CAN_ACTIVATE);
	for (int i = 0; i < lengthof(treeTypes); ++i) {
		addStringItem(_treeItems, treeTypes[i].name, treeTypes[i].id);
	}
	addStringItem(_fileItems, "New", "new");
	addStringItem(_fileItems, "Load", "load");
	addStringItem(_fileItems, "Save", "save");
	addStringItem(_fileItems, "Load Animation", "animation_load");
	addStringItem(_fileItems, "Save Animation", "animation_save");
	addStringItem(_fileItems, "Prefab", "prefab");
	addStringItem(_fileItems, "Heightmap", "importheightmap");
	addStringItem(_fileItems, "Image as Plane", "importplane");
	addStringItem(_fileItems, "Quit", "quit");

	addStringItem(_structureItems, "Trees")->sub_source = &_treeItems;
}

VoxEditWindow::~VoxEditWindow() {
	if (tb::TBSelectDropdown* dropdown = getWidgetByType<tb::TBSelectDropdown>("animationlist")) {
		dropdown->setSource(nullptr);
	}
}

bool VoxEditWindow::init() {
	_layerSettings.reset();
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
	_layerWidget = getWidgetByType<LayerWidget>("layercontainer");
	if (_layerWidget == nullptr) {
		Log::error("Failed to init the main window: Could not get the layer node with id 'layercontainer'");
		return false;
	}
	const int index = _paletteWidget->getValue();
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
	voxedit::sceneMgr().modifier().setCursorVoxel(voxel);
	_paletteWidget->markAsClean();

	_sceneTop = getWidgetByType<Viewport>("editorscenetop");
	_sceneLeft = getWidgetByType<Viewport>("editorsceneleft");
	_sceneFront = getWidgetByType<Viewport>("editorscenefront");
	_sceneAnimation = getWidgetByType<Viewport>("animationscene");
	_animationWidget = getWidgetByType<tb::TBLayout>("animationwidget");

	if (tb::TBSelectDropdown* dropdown = getWidgetByType<tb::TBSelectDropdown>("animationlist")) {
		dropdown->setSource(&_animationItems);
		for (int i = 0; i < std::enum_value(animation::Animation::Max); ++i) {
			addStringItem(_animationItems, animation::toString((animation::Animation)i));
		}
	}

	_fourViewAvailable = _sceneTop != nullptr && _sceneLeft != nullptr && _sceneFront != nullptr;
	_animationViewAvailable = _animationWidget != nullptr;

	tb::TBWidget* toggleViewPort = getWidget("toggleviewport");
	if (toggleViewPort != nullptr) {
		toggleViewPort->setState(tb::WIDGET_STATE_DISABLED, !_fourViewAvailable);
	}
	tb::TBWidget* toggleAnimation = getWidget("toggleanimation");
	if (toggleAnimation != nullptr) {
		toggleAnimation->setState(tb::WIDGET_STATE_DISABLED, !_animationViewAvailable);
	}
	_saveButton = getWidget("save");
	_saveAnimationButton = getWidget("animation_save");
	_undoButton = getWidget("undo");
	_redoButton = getWidget("redo");

	_cursorX = getWidgetByType<tb::TBEditField>("cursorx");
	_cursorY = getWidgetByType<tb::TBEditField>("cursory");
	_cursorZ = getWidgetByType<tb::TBEditField>("cursorz");

	_paletteIndex = getWidgetByType<tb::TBEditField>("paletteindex");

	_lockedX = getWidgetByType<tb::TBCheckBox>("lockx");
	_lockedY = getWidgetByType<tb::TBCheckBox>("locky");
	_lockedZ = getWidgetByType<tb::TBCheckBox>("lockz");

	_translateX = getWidgetByType<tb::TBInlineSelect>("translatex");
	_translateY = getWidgetByType<tb::TBInlineSelect>("translatey");
	_translateZ = getWidgetByType<tb::TBInlineSelect>("translatez");

	_mirrorAxisNone = getWidgetByType<tb::TBRadioButton>("mirroraxisnone");
	_mirrorAxisX = getWidgetByType<tb::TBRadioButton>("mirroraxisx");
	_mirrorAxisY = getWidgetByType<tb::TBRadioButton>("mirroraxisy");
	_mirrorAxisZ = getWidgetByType<tb::TBRadioButton>("mirroraxisz");

	_placeModifier = getWidgetByType<tb::TBRadioButton>("actionplace");
	_deleteModifier = getWidgetByType<tb::TBRadioButton>("actiondelete");
	_selectModifier = getWidgetByType<tb::TBRadioButton>("actionselect");
	_overrideModifier = getWidgetByType<tb::TBRadioButton>("actionoverride");
	_colorizeModifier = getWidgetByType<tb::TBRadioButton>("actioncolorize");

	_showAABB = getWidgetByType<tb::TBCheckBox>("optionshowaabb");
	_showGrid = getWidgetByType<tb::TBCheckBox>("optionshowgrid");
	_voxelSize = getWidgetByType<tb::TBInlineSelect>("optionvoxelsize");
	_showAxis = getWidgetByType<tb::TBCheckBox>("optionshowaxis");
	_showLockAxis = getWidgetByType<tb::TBCheckBox>("optionshowlockaxis");
	_renderShadow = getWidgetByType<tb::TBCheckBox>("optionrendershadow");
	if (_showAABB == nullptr || _showGrid == nullptr || _showLockAxis == nullptr
	 || _showAxis == nullptr || _renderShadow == nullptr || _voxelSize == nullptr) {
		Log::error("Could not load all required widgets");
		return false;
	}

	SceneManager& mgr = sceneMgr();
	render::GridRenderer& gridRenderer = mgr.gridRenderer();
	gridRenderer.setRenderAABB(_showAABB->getValue() != 0);
	gridRenderer.setRenderGrid(_showGrid->getValue() != 0);
	mgr.setGridResolution(_voxelSize->getValue());
	mgr.setRenderAxis(_showAxis->getValue() != 0);
	mgr.setRenderLockAxis(_showLockAxis->getValue() != 0);
	mgr.setRenderShadow(_renderShadow->getValue() != 0);

	_lastOpenedFile = core::Var::get(cfg::VoxEditLastFile, "");
	if (mgr.load(_lastOpenedFile->strVal())) {
		afterLoad(_lastOpenedFile->strVal());
	} else {
		voxel::Region region = _layerSettings.region();
		if (region.isValid()) {
			_layerSettings.reset();
			region = _layerSettings.region();
		}
		if (!mgr.newScene(true, _layerSettings.name, region)) {
			return false;
		}
		afterLoad("");
	}
	_scene->setFocus(tb::WIDGET_FOCUS_REASON_UNKNOWN);

	return true;
}

void VoxEditWindow::updateStatusBar() {
	if (tb::TBTextField* dimension = getWidgetByIDAndType<tb::TBTextField>("dimension")) {
		const int layerIdx = voxedit::sceneMgr().layerMgr().activeLayer();
		const voxel::RawVolume* v = voxedit::sceneMgr().volume(layerIdx);
		const voxel::Region& region = v->region();
		tb::TBStr str;
		const glm::ivec3& mins = region.getLowerCorner();
		const glm::ivec3& maxs = region.getUpperCorner();
		str.setFormatted("%i:%i:%i / %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		dimension->setText(str);
	}
	if (tb::TBTextField* status = getWidgetByIDAndType<tb::TBTextField>("status")) {
		const voxedit::Modifier& modifier = voxedit::sceneMgr().modifier();
		if (modifier.aabbMode()) {
			tb::TBStr str;
			const glm::ivec3& dim = modifier.aabbDim();
			str.setFormatted("w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
			status->setText(str);
		} else if (!_lastExecutedCommand.empty()) {
			const video::WindowedApp* app = video::WindowedApp::getInstance();
			std::string statusText;
			const std::string& keybindingStr = app->getKeyBindingsString(_lastExecutedCommand.c_str());
			if (keybindingStr.empty()) {
				statusText = core::string::format("%s: %s", tr("Command"), _lastExecutedCommand.c_str());
			} else {
				statusText = core::string::format("%s: %s (%s)", tr("Command"), _lastExecutedCommand.c_str(), keybindingStr.c_str());
			}
			status->setText(statusText.c_str());
			_lastExecutedCommand.clear();
		}
	}
}

void VoxEditWindow::update() {
	updateStatusBar();
	_scene->update();
	if (_sceneTop != nullptr) {
		_sceneTop->update();
	}
	if (_sceneLeft != nullptr) {
		_sceneLeft->update();
	}
	if (_sceneFront != nullptr) {
		_sceneFront->update();
	}
	if (_sceneAnimation != nullptr) {
		_sceneAnimation->update();
	}
	if (_paletteWidget != nullptr) {
		_paletteWidget->setVoxelColor(sceneMgr().hitCursorVoxel().getColor());
	}
}

bool VoxEditWindow::isLayerWidgetDropTarget() const {
	return tb::TBWidget::hovered_widget == _layerWidget;
}

bool VoxEditWindow::isPaletteWidgetDropTarget() const {
	return tb::TBWidget::hovered_widget == _paletteWidget;
}

bool VoxEditWindow::isSceneHovered() const {
	return tb::TBWidget::hovered_widget == _scene
			|| tb::TBWidget::hovered_widget == _sceneTop
			|| tb::TBWidget::hovered_widget == _sceneLeft
			|| tb::TBWidget::hovered_widget == _sceneFront
			|| tb::TBWidget::hovered_widget == _sceneAnimation;
}

void VoxEditWindow::toggleViewport() {
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

	const tb::WIDGET_VISIBILITY visibility = vis ? tb::WIDGET_VISIBILITY_GONE : tb::WIDGET_VISIBILITY_VISIBLE;
	if (_sceneTop != nullptr) {
		_sceneTop->setVisibility(visibility);
	}
	if (_sceneLeft != nullptr) {
		_sceneLeft->setVisibility(visibility);
	}
	if (_sceneFront != nullptr) {
		_sceneFront->setVisibility(visibility);
	}
}

void VoxEditWindow::toggleAnimation() {
	if (_animationWidget != nullptr) {
		const bool vis = _animationWidget->getVisibilityCombined();
		_animationWidget->setVisibility(vis ? tb::WIDGET_VISIBILITY_GONE : tb::WIDGET_VISIBILITY_VISIBLE);
	}
}

bool VoxEditWindow::handleEvent(const tb::TBWidgetEvent &ev) {
	// ui actions with direct command bindings
	static const char *ACTIONS[] = {
		"new", "quit", "load", "animation_load", "animation_save",
		"prefab", "save", "importheightmap", "importplane", nullptr
	};

	for (const char** action = ACTIONS; *action != nullptr; ++action) {
		if (ev.isAny(TBIDC(*action))) {
			core::Command::execute("%s", *action);
			_lastExecutedCommand = *action;
			return true;
		}
	}
	if (ev.isAny(TBIDC("menu_structure"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("structure_popup"))) {
			menu->show(&_structureItems, tb::TBPopupAlignment());
		}
	} else if (ev.isAny(TBIDC("menu_tree"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("tree_popup"))) {
			menu->show(&_treeItems, tb::TBPopupAlignment());
		}
	} else if (ev.isAny(TBIDC("menu_file"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("menu_file_window"))) {
			menu->show(&_fileItems, tb::TBPopupAlignment());
		}
	} else if (ev.isAny(TBIDC("dialog_noise"))) {
		new NoiseWindow(this);
	} else if (ev.isAny(TBIDC("optionshowgrid"))) {
		sceneMgr().gridRenderer().setRenderGrid(ev.target->getValue() == 1);
	} else if (ev.isAny(TBIDC("shiftvolumes"))) {
		const int x = _translateX != nullptr ? _translateX->getValue() : 1;
		const int y = _translateY != nullptr ? _translateY->getValue() : 1;
		const int z = _translateZ != nullptr ? _translateZ->getValue() : 1;
		sceneMgr().shift(x, y, z);
	} else if (ev.isAny(TBIDC("movevoxels"))) {
		const int x = _translateX != nullptr ? _translateX->getValue() : 1;
		const int y = _translateY != nullptr ? _translateY->getValue() : 1;
		const int z = _translateZ != nullptr ? _translateZ->getValue() : 1;
		sceneMgr().move(x, y, z);
	} else if (ev.isAny(TBIDC("optionshowaxis"))) {
		sceneMgr().setRenderAxis(ev.target->getValue() == 1);
	} else if (ev.isAny(TBIDC("optionshowlockaxis"))) {
		sceneMgr().setRenderLockAxis(ev.target->getValue() == 1);
	} else if (ev.isAny(TBIDC("optionshowaabb"))) {
		sceneMgr().gridRenderer().setRenderAABB(ev.target->getValue() == 1);
	} else if (ev.isAny(TBIDC("optionrendershadow"))) {
		sceneMgr().setRenderShadow(ev.target->getValue() == 1);
	} else {
		return false;
	}
	return true;
}

bool VoxEditWindow::handleClickEvent(const tb::TBWidgetEvent &ev) {
	tb::TBWidget *widget = ev.target;
	const tb::TBID &id = widget->getID();

	if (id == TBIDC("new_scene")) {
		if (ev.ref_id == TBIDC("ok")) {
			const voxel::Region& region = _layerSettings.region();
			if (region.isValid()) {
				if (!voxedit::sceneMgr().newScene(true, _layerSettings.name, region)) {
					return false;
				}
				afterLoad("");
			} else {
				popup(tr("Invalid dimensions"), tr("The layer dimensions are not valid?"), PopupType::Ok);
				_layerSettings.reset();
			}
			return true;
		}
	}
	if (id == TBIDC("scene_settings_open")) {
		auto &renderer = sceneMgr().renderer();
		auto &shadow = renderer.shadow();
		_settings = SceneSettings();
		_settings.ambientColor = core::Var::getSafe(cfg::VoxEditAmbientColor)->vec3Val();
		_settings.diffuseColor = core::Var::getSafe(cfg::VoxEditDiffuseColor)->vec3Val();
		_settings.sunPosition = shadow.sunPosition();
		_settings.sunDirection = shadow.sunDirection();
		SceneSettingsWindow* settings = new SceneSettingsWindow(this, &_settings);
		if (!settings->show()) {
			delete settings;
		}
	} else if (id == TBIDC("loadpalette")) {
		tb::TBPoint rootPos(ev.target_x, ev.target_y);
		ev.target->convertToRoot(rootPos.x, rootPos.y);
		PaletteSelector* selector = new PaletteSelector(this);
		selector->setPosition(rootPos);
	} else if (id == TBIDC("scene_settings") && ev.ref_id == TBIDC("ok")) {
		auto &renderer = sceneMgr().renderer();
		if (_settings.ambientDirty) {
			const std::string& c = core::string::format("%f %f %f", _settings.ambientColor.x, _settings.ambientColor.y, _settings.ambientColor.z);
			core::Var::getSafe(cfg::VoxEditAmbientColor)->setVal(c);
		}
		if (_settings.diffuseDirty) {
			const std::string& c = core::string::format("%f %f %f", _settings.diffuseColor.x, _settings.diffuseColor.y, _settings.diffuseColor.z);
			core::Var::getSafe(cfg::VoxEditDiffuseColor)->setVal(c);
		}
		if (_settings.sunPositionDirty) {
			renderer.setSunPosition(_settings.sunPosition, glm::zero<glm::vec3>(), glm::up);
		}
		if (_settings.sunDirectionDirty) {
			// TODO: sun direction
		}
		return true;
	}
	if (id == TBIDC("unsaved_changes_new")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			LayerWindowSettings s;
			s.type = LayerWindowType::NewScene;
			LayerWindow* win = new LayerWindow(this, TBIDC("new_scene"), _layerSettings, &s);
			if (!win->show()) {
				delete win;
			}
		}
		return true;
	}
	if (id == TBIDC("unsaved_changes_quit")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			close();
		}
		return true;
	}
	if (id == TBIDC("unsaved_changes_load")) {
		if (ev.ref_id == TBIDC("TBMessageWindow.yes")) {
			sceneMgr().load(_loadFile);
			afterLoad(_loadFile);
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

	return false;
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
		return false;
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
		return false;
	} else if (id == TBIDC("shader")) {
		tb::TBWidget *parent = widget->getParent();
		if (Viewport *viewport = parent->safeCastTo<Viewport>()) {
			const int value = widget->getValue();
			voxedit::ViewportController::ShaderType type = voxedit::ViewportController::ShaderType::None;
			if (value == 1) {
				type = voxedit::ViewportController::ShaderType::Edge;
			}
			viewport->controller().setShaderType(type);
			return true;
		}
		return false;
	} else if (id == TBIDC("shapetype")) {
		sceneMgr().modifier().setShapeType((voxedit::ShapeType)widget->getValue());
		return true;
	} else if (ev.isAny(TBIDC("animationlist"))) {
		sceneMgr().animationEntity().setAnimation((animation::Animation)widget->getValue());
		return true;
	} else if (id == TBIDC("optionvoxelsize")) {
		sceneMgr().setGridResolution(widget->getValue());
		return true;
	} else if (id == TBIDC("lockx")) {
		sceneMgr().setLockedAxis(math::Axis::X, widget->getValue() != 1);
		return true;
	} else if (id == TBIDC("locky")) {
		sceneMgr().setLockedAxis(math::Axis::Y, widget->getValue() != 1);
		return true;
	} else if (id == TBIDC("lockz")) {
		sceneMgr().setLockedAxis(math::Axis::Z, widget->getValue() != 1);
		return true;
	} else if (id == TBIDC("cursorx")) {
		const tb::TBStr& str = widget->getText();
		if (str.isEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = sceneMgr().cursorPosition();
		pos.x = val;
		sceneMgr().setCursorPosition(pos, true);
		return true;
	} else if (id == TBIDC("cursory")) {
		const tb::TBStr& str = widget->getText();
		if (str.isEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = sceneMgr().cursorPosition();
		pos.y = val;
		sceneMgr().setCursorPosition(pos, true);
		return true;
	} else if (id == TBIDC("cursorz")) {
		const tb::TBStr& str = widget->getText();
		if (str.isEmpty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = sceneMgr().cursorPosition();
		pos.z = val;
		sceneMgr().setCursorPosition(pos, true);
		return true;
	}

	static const char *ACTIONS[] = {
		"mirroraxisx", "mirroraxisy", "mirroraxisz", "mirroraxisnone", "actionselect",
		"actionplace", "actiondelete", "actioncolorize", "actionoverride",
		nullptr
	};
	for (const char** action = ACTIONS; *action != nullptr; ++action) {
		if (ev.isAny(TBIDC(*action)) && widget->getValue() == 1) {
			core::Command::execute("%s", *action);
			_lastExecutedCommand = *action;
			return true;
		}
	}

	animation::SkeletonAttribute* skeletonAttributes = sceneMgr().skeletonAttributes();
	for (const animation::SkeletonAttributeMeta* metaIter = skeletonAttributes->metaArray(); metaIter->name; ++metaIter) {
		const animation::SkeletonAttributeMeta& meta = *metaIter;
		if (id == TBIDC(meta.name)) {
			const float val = (float)ev.target->getValueDouble();
			float *saVal = (float*)(((uint8_t*)skeletonAttributes) + meta.offset);
			*saVal = val;
			break;
		}
	}

	return false;
}

void VoxEditWindow::onProcess() {
	Super::onProcess();

	const Modifier& modifier = sceneMgr().modifier();
	if (_paletteWidget->isDirty()) {
		const int index = _paletteWidget->getValue();
		const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
		sceneMgr().modifier().setCursorVoxel(voxel);
		_paletteWidget->markAsClean();
	} else {
		const voxel::Voxel& voxel = modifier.cursorVoxel();
		if (!voxel::isAir(voxel.getMaterial())) {
			_paletteWidget->setValue(voxel.getColor());
		}
	}
	const ModifierType modifierType = modifier.modifierType();
	constexpr ModifierType overrideType = ModifierType::Delete | ModifierType::Place;
	if ((modifierType & overrideType) == overrideType) {
		if (_overrideModifier) {
			_overrideModifier->setValue(1);
		}
	} else if ((modifierType & ModifierType::Place) == ModifierType::Place) {
		if (_placeModifier) {
			_placeModifier->setValue(1);
		}
	} else if ((modifierType & ModifierType::Select) == ModifierType::Select) {
		if (_selectModifier) {
			_selectModifier->setValue(1);
		}
	} else if ((modifierType & ModifierType::Delete) == ModifierType::Delete) {
		if (_deleteModifier) {
			_deleteModifier->setValue(1);
		}
	} else if ((modifierType & ModifierType::Update) == ModifierType::Update) {
		if (_colorizeModifier) {
			_colorizeModifier->setValue(1);
		}
	}

	if (_paletteIndex != nullptr) {
		static int index = -1;
		voxel::Voxel voxel = sceneMgr().modifier().cursorVoxel();
		const int newIndex = voxel.getColor();
		if (index != newIndex) {
			index = newIndex;
			_paletteIndex->setTextFormatted("Color index: %u", voxel.getColor());
		}
	}

	if (_saveAnimationButton != nullptr) {
		_saveAnimationButton->setState(tb::WIDGET_STATE_DISABLED, sceneMgr().empty() || sceneMgr().editMode() == EditMode::Volume);
	}
	if (_saveButton != nullptr) {
		_saveButton->setState(tb::WIDGET_STATE_DISABLED, sceneMgr().empty());
	}
	if (_undoButton != nullptr) {
		_undoButton->setState(tb::WIDGET_STATE_DISABLED, !sceneMgr().mementoHandler().canUndo());
	}
	if (_redoButton != nullptr) {
		_redoButton->setState(tb::WIDGET_STATE_DISABLED, !sceneMgr().mementoHandler().canRedo());
	}
	const glm::ivec3& pos = sceneMgr().cursorPosition();
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

	const math::Axis lockedAxis = sceneMgr().lockedAxis();
	if (_lockedX != nullptr) {
		_lockedX->setValue((lockedAxis & math::Axis::X) != math::Axis::None);
	}
	if (_lockedY != nullptr) {
		_lockedY->setValue((lockedAxis & math::Axis::Y) != math::Axis::None);
	}
	if (_lockedZ != nullptr) {
		_lockedZ->setValue((lockedAxis & math::Axis::Z) != math::Axis::None);
	}

	const math::Axis mirrorAxis = modifier.mirrorAxis();
	if (_mirrorAxisNone != nullptr) {
		_mirrorAxisNone->setValue(mirrorAxis == math::Axis::None);
	}
	if (_mirrorAxisX != nullptr) {
		_mirrorAxisX->setValue(mirrorAxis == math::Axis::X);
	}
	if (_mirrorAxisY != nullptr) {
		_mirrorAxisY->setValue(mirrorAxis == math::Axis::Y);
	}
	if (_mirrorAxisZ != nullptr) {
		_mirrorAxisZ->setValue(mirrorAxis == math::Axis::Z);
	}
}

bool VoxEditWindow::onEvent(const tb::TBWidgetEvent &ev) {
	if (ev.type == tb::EVENT_TYPE_CUSTOM) {
		if (handleEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_COMMAND) {
		_lastExecutedCommand = ev.string;
		return true;
	} else if (ev.type == tb::EVENT_TYPE_CLICK) {
		if (handleClickEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_CHANGED) {
		if (handleChangeEvent(ev)) {
			return true;
		}
	} else if (ev.type == tb::EVENT_TYPE_SHORTCUT) {
		static const char *ACTIONS[] = {
			"undo", "redo", nullptr
		};

		for (const char** action = ACTIONS; *action != nullptr; ++action) {
			if (ev.ref_id == TBIDC(*action)) {
				core::Command::execute("%s", *action);
				_lastExecutedCommand = *action;
				return true;
			}
		}
	}

	return Super::onEvent(ev);
}

void VoxEditWindow::onDie() {
	Super::onDie();
	if (tb::TBSelectDropdown* dropdown = getWidgetByType<tb::TBSelectDropdown>("animationlist")) {
		dropdown->setSource(nullptr);
	}

	// TODO: we should get a chance here to ask - really sure? if we have unsaved data...
	requestQuit();
}

void VoxEditWindow::quit() {
	if (sceneMgr().dirty()) {
		popup(tr("Unsaved Modifications"),
				tr("There are unsaved modifications.\nDo you wish to discard them and quit?"),
				PopupType::YesNo, "unsaved_changes_quit");
		return;
	}
	close();
}

bool VoxEditWindow::importAsPlane(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string file) { importAsPlane(file); }, "png");
		return true;
	}

	return sceneMgr().importAsPlane(file);
}

bool VoxEditWindow::importPalette(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string file) { importPalette(file); }, "png");
		return true;
	}

	return sceneMgr().importPalette(file);
}

bool VoxEditWindow::importHeightmap(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string file) { importHeightmap(file); }, "png");
		return true;
	}

	return sceneMgr().importHeightmap(file);
}

bool VoxEditWindow::save(const std::string& file) {
	if (file.empty()) {
		getApp()->saveDialog([this] (const std::string uifile) {save(uifile); }, SUPPORTED_VOXEL_FORMATS_SAVE);
		return true;
	}
	if (!sceneMgr().save(file)) {
		Log::warn("Failed to save the model");
		popup(tr("Error"), tr("Failed to save the model"));
		return false;
	}
	Log::info("Saved the model to %s", file.c_str());
	_lastOpenedFile->setVal(file);
	return true;
}

bool VoxEditWindow::saveScreenshot(const std::string& file) {
	if (file.empty()) {
		getApp()->saveDialog([this] (const std::string file) {saveScreenshot(file); }, "png");
		return true;
	}
	if (!_scene->saveImage(file.c_str())) {
		Log::warn("Failed to save screenshot");
		return false;
	}
	Log::info("Screenshot created at '%s'", file.c_str());
	return true;
}

void VoxEditWindow::resetCamera() {
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
	if (_sceneAnimation != nullptr) {
		_sceneAnimation->resetCamera();
	}
}

bool VoxEditWindow::prefab(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string file) { prefab(file); }, SUPPORTED_VOXEL_FORMATS_LOAD);
		return true;
	}

	return sceneMgr().prefab(file);
}

void VoxEditWindow::afterLoad(const std::string& file) {
	_lastOpenedFile->setVal(file);
	resetCamera();
}

bool VoxEditWindow::loadAnimationEntity(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string file) { std::string copy(file); loadAnimationEntity(copy); }, "lua");
		return true;
	}
	if (!sceneMgr().loadAnimationEntity(file)) {
		return false;
	}
	resetCamera();
	if (tb::TBClickLabel* toggleAnimation = getWidgetByIDAndType<tb::TBClickLabel>("toggleanimationlabel")) {
		if (!toggleAnimation->getState(tb::WIDGET_STATE_PRESSED)) {
			tb::TBWidgetEvent target_ev(tb::EVENT_TYPE_CLICK);
			target_ev.ref_id = toggleAnimation->getID();
			toggleAnimation->invokeEvent(target_ev);
		}
	}
	if (tb::TBLayout* layout = getWidgetByType<tb::TBLayout>("animationsettings")) {
		layout->deleteAllChildren();
		const animation::SkeletonAttribute* skeletonAttributes = sceneMgr().skeletonAttributes();
		for (const animation::SkeletonAttributeMeta* metaIter = skeletonAttributes->metaArray(); metaIter->name; ++metaIter) {
			const animation::SkeletonAttributeMeta& meta = *metaIter;
			tb::TBLayout *innerLayout = new tb::TBLayout();
			tb::TBTextField *name = new tb::TBTextField();
			name->setText(meta.name);
			tb::TBInlineSelectDouble *value = new tb::TBInlineSelectDouble();
			const float *saVal = (const float*)(((const uint8_t*)skeletonAttributes) + meta.offset);
			value->setValueDouble(*saVal);
			value->setID(TBIDC(meta.name));
			value->setLimits(-100.0, 100.0);
			innerLayout->addChild(name);
			innerLayout->addChild(value);
			innerLayout->setAxis(tb::AXIS_X);
			innerLayout->setGravity(tb::WIDGET_GRAVITY_ALL);
			innerLayout->setLayoutDistribution(tb::LAYOUT_DISTRIBUTION_AVAILABLE);
			layout->addChild(innerLayout);
		}
	}
	return true;
}

bool VoxEditWindow::load(const std::string& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const std::string file) { std::string copy(file); load(copy); }, SUPPORTED_VOXEL_FORMATS_LOAD);
		return true;
	}

	if (!sceneMgr().dirty()) {
		if (sceneMgr().load(file)) {
			afterLoad(file);
			return true;
		}
		return false;
	}

	_loadFile = file;
	popup(tr("Unsaved Modifications"),
			tr("There are unsaved modifications.\nDo you wish to discard them and load?"),
			PopupType::YesNo, "unsaved_changes_load");
	return false;
}

bool VoxEditWindow::createNew(bool force) {
	if (!force && sceneMgr().dirty()) {
		popup(tr("Unsaved Modifications"),
				tr("There are unsaved modifications.\nDo you wish to discard them and close?"),
				PopupType::YesNo, "unsaved_changes_new");
	} else {
		LayerWindowSettings s;
		s.type = LayerWindowType::NewScene;
		LayerWindow* win = new LayerWindow(this, TBIDC("new_scene"), _layerSettings, &s);
		if (!win->show()) {
			delete win;
		}
	}
	return false;
}

}
