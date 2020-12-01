/**
 * @file
 */

#include "VoxEditWindow.h"
#include "core/collection/DynamicArray.h"
#include "palette/PaletteWidget.h"
#include "palette/PaletteSelector.h"
#include "io/Filesystem.h"
#include "tb_editfield.h"
#include "tb_hash.h"
#include "tb_layout.h"
#include "tb_widgets.h"
#include "tb_widgets_common.h"
#include "video/WindowedApp.h"
#include "core/Var.h"
#include "core/StringUtil.h"
#include "command/Command.h"
#include "editorscene/Viewport.h"
#include "settings/SceneSettingsWindow.h"
#include "layer/LayerWidget.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/VolumeFormat.h"
#include "ui/turbobadger/UIApp.h"

static PaletteWidgetFactory paletteWidget_wf;
static LayerWidgetFactory layerWidget_wf;
static ViewportFactory viewport_wf;

namespace voxedit {

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

VoxEditWindow::VoxEditWindow(::ui::turbobadger::UIApp* tool) :
		Super(tool), AbstractMainWindow(tool) {
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
}

VoxEditWindow::~VoxEditWindow() {
	if (tb::TBSelectDropdown* dropdown = getWidgetByType<tb::TBSelectDropdown>("animationlist")) {
		dropdown->setSource(nullptr);
	}
	if (tb::TBSelectDropdown* dropdown = getWidgetByType<tb::TBSelectDropdown>("treetype")) {
		dropdown->setSource(nullptr);
	}
	if (tb::TBSelectDropdown* dropdown = getWidgetByType<tb::TBSelectDropdown>("scripttype")) {
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
		for (int i = core::enumVal(animation::Animation::MIN); i <= core::enumVal(animation::Animation::MAX); ++i) {
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

	_octaves     = getWidgetByType<tb::TBInlineSelect>("noise_octaves");
	_frequency   = getWidgetByType<tb::TBInlineSelectDouble>("noise_frequency");
	_lacunarity  = getWidgetByType<tb::TBInlineSelectDouble>("noise_lacunarity");
	_gain        = getWidgetByType<tb::TBInlineSelectDouble>("noise_gain");
	_noiseSection = getWidgetByID("noisesection");

	if (_octaves == nullptr || _frequency == nullptr || _lacunarity == nullptr || _gain == nullptr) {
		Log::error("Not all needed widgets were found");
		return false;
	}

	_lsystemAxiom = getWidgetByType<tb::TBEditField>("lsystem_axiom");
	_lsystemRules = getWidgetByType<tb::TBEditField>("lsystem_rules");
	_lsystemAngle = getWidgetByType<tb::TBInlineSelectDouble>("lsystem_angle");
	_lsystemLength = getWidgetByType<tb::TBInlineSelectDouble>("lsystem_length");
	_lsystemWidth = getWidgetByType<tb::TBInlineSelectDouble>("lsystem_width");
	_lsystemWidthIncrement = getWidgetByType<tb::TBInlineSelectDouble>("lsystem_widthincrement");
	_lsystemLeavesRadius = getWidgetByType<tb::TBInlineSelectDouble>("lsystem_leavesradius");
	_lsystemIterations = getWidgetByType<tb::TBInlineSelect>("lsystem_iterations");
	_lsystemSection = getWidgetByID("lsystemsection");

	if (_lsystemAxiom == nullptr || _lsystemRules == nullptr || _lsystemAngle == nullptr || _lsystemLength == nullptr ||
		_lsystemWidth == nullptr || _lsystemWidthIncrement == nullptr || _lsystemIterations == nullptr || _lsystemSection == nullptr ||
		_lsystemLeavesRadius == nullptr) {
		Log::error("Not all needed lsystem widgets were found");
		return false;
	}

	_treeType = getWidgetByType<tb::TBSelectDropdown>("treetype");
	if (_treeType == nullptr) {
		Log::error("treetype widget not found");
		return false;
	}

	_treeAutoGenerateOnChange = getWidgetByType<tb::TBCheckBox>("treeautogen");

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
#endif
#define TREECONFIG(configType, variableName, parameterType, treeType, widget) TreeWidget{offsetof(configType, variableName), parameterType, treeType, widget}
#define TREECONFIGINT(configType, variableName, treeType, text) TREECONFIG(configType, variableName, TreeParameterWidgetType::Int, treeType, createTreeParameterWidget(TreeParameterWidgetType::Int, treeParameterLayout, "treeconfig-" #variableName, text))
#define TREECONFIGFLOAT(configType, variableName, treeType, text) TREECONFIG(configType, variableName, TreeParameterWidgetType::Float, treeType, createTreeParameterWidget(TreeParameterWidgetType::Float, treeParameterLayout, "treeconfig-" #variableName, text))

	tb::TBLayout* treeParameterLayout = getWidgetByIDAndType<tb::TBLayout>("treeparameterlayout");
	if (treeParameterLayout != nullptr) {
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeConfig, seed, voxelgenerator::TreeType::Max, "Seed"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeConfig, trunkStrength, voxelgenerator::TreeType::Max, "Trunk strength"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeConfig, trunkHeight, voxelgenerator::TreeType::Max, "Trunk height"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeConfig, leavesWidth, voxelgenerator::TreeType::Max, "Leaves width"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeConfig, leavesHeight, voxelgenerator::TreeType::Max, "Leaves height"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeConfig, leavesDepth, voxelgenerator::TreeType::Max, "Leaves depth"));

		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeBranchEllipsis, branchLength, voxelgenerator::TreeType::BranchesEllipsis, "Branch length"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeBranchEllipsis, branchHeight, voxelgenerator::TreeType::BranchesEllipsis, "Branch height"));

		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePalm, branchSize, voxelgenerator::TreeType::Palm, "Branch size"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePalm, trunkWidth, voxelgenerator::TreeType::Palm, "Trunk width"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePalm, trunkDepth, voxelgenerator::TreeType::Palm, "Trunk depth"));
		_treeWidgets.insert(TREECONFIGFLOAT(voxelgenerator::TreePalm, branchFactor, voxelgenerator::TreeType::Palm, "Branch reduction"));
		_treeWidgets.insert(TREECONFIGFLOAT(voxelgenerator::TreePalm, trunkFactor, voxelgenerator::TreeType::Palm, "Trunk reduction"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePalm, branches, voxelgenerator::TreeType::Palm, "Leaves"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePalm, branchControlOffset, voxelgenerator::TreeType::Palm, "Bezier leaf"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePalm, trunkControlOffset, voxelgenerator::TreeType::Palm, "Bezier trunk"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePalm, randomLeavesHeightOffset, voxelgenerator::TreeType::Palm, "Leaves h-offset"));

		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeFir, branches, voxelgenerator::TreeType::Fir, "Branches"));
		_treeWidgets.insert(TREECONFIGFLOAT(voxelgenerator::TreeFir, w, voxelgenerator::TreeType::Fir, "W"));
		_treeWidgets.insert(TREECONFIGFLOAT(voxelgenerator::TreeFir, branchPositionFactor, voxelgenerator::TreeType::Fir, "Branch position factor"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeFir, amount, voxelgenerator::TreeType::Fir, "Amount"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeFir, branchStrength, voxelgenerator::TreeType::Fir, "Branch strength"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeFir, branchDownwardOffset, voxelgenerator::TreeType::Fir, "Branch downward offset"));

		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePine, startWidth, voxelgenerator::TreeType::Pine, "Start width"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePine, startDepth, voxelgenerator::TreeType::Pine, "Start depth"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePine, singleLeafHeight, voxelgenerator::TreeType::Pine, "Leaf height"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreePine, singleStepDelta, voxelgenerator::TreeType::Pine, "Step delta"));

		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeDomeHanging, branches, voxelgenerator::TreeType::DomeHangingLeaves, "Branches"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeDomeHanging, hangingLeavesLengthMin, voxelgenerator::TreeType::DomeHangingLeaves, "Leaves min length"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeDomeHanging, hangingLeavesLengthMax, voxelgenerator::TreeType::DomeHangingLeaves, "Leaves max length"));
		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeDomeHanging, hangingLeavesThickness, voxelgenerator::TreeType::DomeHangingLeaves, "Leaves thickness"));

		_treeWidgets.insert(TREECONFIGINT(voxelgenerator::TreeSpaceColonization, branchSize, voxelgenerator::TreeType::SpaceColonization, "Branch size"));
		_treeWidgets.insert(TREECONFIGFLOAT(voxelgenerator::TreeSpaceColonization, trunkFactor, voxelgenerator::TreeType::SpaceColonization, "Trunk reduction"));
		_treeSection = getWidgetByID("treesection");
	} else {
		Log::warn("Could not find treeparameterlayout widget");
	}

#undef TREECONFIGFLOAT
#undef TREECONFIGINT
#undef TREECONFIG
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

	_treeType->setSource(&_treeItems);

	_scriptSection = getWidgetByID("scriptsection");
	_scriptType = getWidgetByType<tb::TBSelectDropdown>("scripttype");
	if (_scriptType == nullptr) {
		Log::error("scripttype widget not found");
		return false;
	}
	_scripts = sceneMgr().luaGenerator().listScripts();
	for (const auto& e : _scripts) {
		addStringItem(_scriptItems, e.c_str());
	}
	_scriptType->setSource(&_scriptItems);

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
	((Viewport*)_scene)->setFocus(tb::WIDGET_FOCUS_REASON_UNKNOWN);

	return true;
}

void VoxEditWindow::shutdown() {
}

tb::TBWidget* VoxEditWindow::createTreeParameterWidget(TreeParameterWidgetType type, tb::TBLayout* parent, const char *id, const char *name) {
	tb::TBWidget *value = nullptr;
	if (type == TreeParameterWidgetType::Int) {
		value = new tb::TBInlineSelect();
	}

	if (type == TreeParameterWidgetType::Float) {
		value = new tb::TBInlineSelectDouble();
	}

	tb::TBTextField *text = new tb::TBTextField();
	text->setText(name);

	tb::TBLayout* innerLayout = new tb::TBLayout();
	innerLayout->setID(TBIDC(id));
	innerLayout->addChild(text);
	innerLayout->setAxis(tb::AXIS_Y);
	if (value != nullptr) {
		innerLayout->addChild(value);
	}

	parent->addChild(innerLayout);
	return value;
}

void VoxEditWindow::switchTreeType(voxelgenerator::TreeType treeType) {
	switch (treeType) {
		case voxelgenerator::TreeType::Dome:
			_treeGeneratorContext.dome = voxelgenerator::TreeDome();
			break;
		case voxelgenerator::TreeType::DomeHangingLeaves:
			_treeGeneratorContext.domehanging = voxelgenerator::TreeDomeHanging();
			break;
		case voxelgenerator::TreeType::Cone:
			_treeGeneratorContext.cone = voxelgenerator::TreeCone();
			break;
		case voxelgenerator::TreeType::Ellipsis:
			_treeGeneratorContext.ellipsis = voxelgenerator::TreeEllipsis();
			break;
		case voxelgenerator::TreeType::BranchesEllipsis:
			_treeGeneratorContext.branchellipsis = voxelgenerator::TreeBranchEllipsis();
			break;
		case voxelgenerator::TreeType::Cube:
		case voxelgenerator::TreeType::CubeSideCubes:
			_treeGeneratorContext.cube = voxelgenerator::TreeCube();
			break;
		case voxelgenerator::TreeType::Pine:
			_treeGeneratorContext.pine = voxelgenerator::TreePine();
			break;
		case voxelgenerator::TreeType::Fir:
			_treeGeneratorContext.fir = voxelgenerator::TreeFir();
			break;
		case voxelgenerator::TreeType::Palm:
			_treeGeneratorContext.palm = voxelgenerator::TreePalm();
			break;
		case voxelgenerator::TreeType::SpaceColonization:
			_treeGeneratorContext.spacecolonization = voxelgenerator::TreeSpaceColonization();
			break;
		case voxelgenerator::TreeType::Max:
		default:
			break;
	}
	_treeGeneratorContext.cfg.type = treeType;
	for (TreeWidget& widget : _treeWidgets) {
		if (widget.treeType != voxelgenerator::TreeType::Max && widget.treeType != treeType) {
			widget.widget->getParent()->setVisibility(tb::WIDGET_VISIBILITY_GONE);
			continue;
		}
		widget.widget->getParent()->setVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
		if (widget.type == TreeParameterWidgetType::Int) {
			const int value = *(const int*)((const uint8_t*)&_treeGeneratorContext + widget.ctxOffset);
			widget.widget->setValue(value);
		} else if (widget.type == TreeParameterWidgetType::Float) {
			const float value = *(const float*)((const uint8_t*)&_treeGeneratorContext + widget.ctxOffset);
			widget.widget->setValueDouble(value);
		}
	}
}

void VoxEditWindow::switchScriptType(const core::String& scriptName) {
	Log::info("Switch activate script to %s", scriptName.c_str());
	_activeScript = sceneMgr().luaGenerator().load(scriptName);
	tb::TBLayout* treeParameterLayout = getWidgetByIDAndType<tb::TBLayout>("scriptparameterslayout");
	if (treeParameterLayout != nullptr) {
		treeParameterLayout->deleteAllChildren();
		core::DynamicArray<voxelgenerator::LUAParameterDescription> params;
		sceneMgr().luaGenerator().argumentInfo(_activeScript, params);
		for (const auto& p : params) {
			tb::TBWidget *value = nullptr;
			switch (p.type) {
			case voxelgenerator::LUAParameterType::ColorIndex:
			// TODO: implement paletteselector
			case voxelgenerator::LUAParameterType::Integer: {
				tb::TBInlineSelect *w = new tb::TBInlineSelect();
				w->setLimits(p.minValue, p.maxValue);
				value = w;
				value->setValue(core::string::toInt(p.defaultValue));
				break;
			}
			case voxelgenerator::LUAParameterType::Float: {
				tb::TBInlineSelectDouble *w = new tb::TBInlineSelectDouble();
				w->setLimits(p.minValue, p.maxValue);
				value = w;
				value->setValueDouble(core::string::toDouble(p.defaultValue));
				break;
			}
			case voxelgenerator::LUAParameterType::String:
				value = new tb::TBEditField();
				value->setText(p.defaultValue);
				break;
			case voxelgenerator::LUAParameterType::Boolean:
				value = new tb::TBCheckBox();
				value->setValue(p.defaultValue == "1" || p.defaultValue == "true");
				break;
			case voxelgenerator::LUAParameterType::Max:
				return;
			}

			tb::TBTextField *text = new tb::TBTextField();
			text->setTextAlign(tb::TB_TEXT_ALIGN_LEFT);
			text->setText(p.name);
			text->setSkinBg("Header");
			text->setGravity(tb::WIDGET_GRAVITY::WIDGET_GRAVITY_LEFT_RIGHT);

			tb::TBLayout* innerLayout = new tb::TBLayout();
			innerLayout->addChild(text);
			innerLayout->setGravity(tb::WIDGET_GRAVITY::WIDGET_GRAVITY_LEFT_RIGHT);
			innerLayout->setLayoutDistribution(tb::LAYOUT_DISTRIBUTION::LAYOUT_DISTRIBUTION_GRAVITY);
			innerLayout->setAxis(tb::AXIS_Y);
			if (!p.description.empty()) {
				tb::TBTextField *description = new tb::TBTextField();
				description->setText(p.description);
				description->setSqueezable(true);
				description->setTextAlign(tb::TB_TEXT_ALIGN_LEFT);
				description->setGravity(tb::WIDGET_GRAVITY::WIDGET_GRAVITY_LEFT_RIGHT);

				innerLayout->addChild(description);
			}
			if (value != nullptr) {
				value->setID(TBIDC(p.name.c_str()));
				value->setGravity(tb::WIDGET_GRAVITY::WIDGET_GRAVITY_LEFT_RIGHT);
				innerLayout->addChild(value);
			}

			treeParameterLayout->addChild(innerLayout);
		}
	}
}

void VoxEditWindow::updateStatusBar() {
	if (tb::TBTextField* dimension = getWidgetByIDAndType<tb::TBTextField>("dimension")) {
		const int layerIdx = voxedit::sceneMgr().layerMgr().activeLayer();
		const voxel::RawVolume* v = voxedit::sceneMgr().volume(layerIdx);
		const voxel::Region& region = v->region();
		const glm::ivec3& mins = region.getLowerCorner();
		const glm::ivec3& maxs = region.getUpperCorner();
		const core::String& str = core::string::format("%i:%i:%i / %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
		dimension->setText(str.c_str());
	}
	if (tb::TBTextField* status = getWidgetByIDAndType<tb::TBTextField>("status")) {
		const voxedit::ModifierFacade& modifier = voxedit::sceneMgr().modifier();
		if (modifier.aabbMode()) {
			const glm::ivec3& dim = modifier.aabbDim();
			const core::String& str = core::string::format("w: %i, h: %i, d: %i", dim.x, dim.y, dim.z);
			status->setText(str.c_str());
		} else if (!_lastExecutedCommand.empty()) {
			const video::WindowedApp* app = video::WindowedApp::getInstance();
			core::String statusText;
			const core::String& keybindingStr = app->getKeyBindingsString(_lastExecutedCommand.c_str());
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
	return tb::TBWidget::hovered_widget == (tb::TBWidget*)_scene
			|| tb::TBWidget::hovered_widget == (tb::TBWidget*)_sceneTop
			|| tb::TBWidget::hovered_widget == (tb::TBWidget*)_sceneLeft
			|| tb::TBWidget::hovered_widget == (tb::TBWidget*)_sceneFront
			|| tb::TBWidget::hovered_widget == (tb::TBWidget*)_sceneAnimation;
}

void VoxEditWindow::toggleViewport() {
	bool vis = false;
	if (_sceneTop != nullptr) {
		vis = ((Viewport*)_sceneTop)->getVisibilityCombined();
	}
	if (!vis && _sceneLeft != nullptr) {
		vis = ((Viewport*)_sceneLeft)->getVisibilityCombined();
	}
	if (!vis && _sceneFront != nullptr) {
		vis = ((Viewport*)_sceneFront)->getVisibilityCombined();
	}

	const tb::WIDGET_VISIBILITY visibility = vis ? tb::WIDGET_VISIBILITY_GONE : tb::WIDGET_VISIBILITY_VISIBLE;
	if (_sceneTop != nullptr) {
		((Viewport*)_sceneTop)->setVisibility(visibility);
	}
	if (_sceneLeft != nullptr) {
		((Viewport*)_sceneLeft)->setVisibility(visibility);
	}
	if (_sceneFront != nullptr) {
		((Viewport*)_sceneFront)->setVisibility(visibility);
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
			command::Command::execute("%s", *action);
			_lastExecutedCommand = *action;
			return true;
		}
	}
	if (ev.isAny(TBIDC("menu_tree"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("tree_popup"))) {
			menu->show(&_treeItems, tb::TBPopupAlignment());
		}
	} else if (ev.isAny(TBIDC("menu_file"))) {
		if (tb::TBMenuWindow *menu = new tb::TBMenuWindow(ev.target, TBIDC("menu_file_window"))) {
			menu->show(&_fileItems, tb::TBPopupAlignment());
		}
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

	if (id == TBIDC("show_tree_panel") && _treeSection != nullptr) {
		if (_treeSection->getVisibility() == tb::WIDGET_VISIBILITY_GONE) {
			_treeSection->setVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
		} else {
			_treeSection->setVisibility(tb::WIDGET_VISIBILITY_GONE);
		}
	}

	if (id == TBIDC("show_script_panel") && _scriptSection != nullptr) {
		if (_scriptSection->getVisibility() == tb::WIDGET_VISIBILITY_GONE) {
			_scriptSection->setVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
		} else {
			_scriptSection->setVisibility(tb::WIDGET_VISIBILITY_GONE);
		}
	}

	if (id == TBIDC("show_noise_panel") && _noiseSection != nullptr) {
		if (_noiseSection->getVisibility() == tb::WIDGET_VISIBILITY_GONE) {
			_noiseSection->setVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
		} else {
			_noiseSection->setVisibility(tb::WIDGET_VISIBILITY_GONE);
		}
	}

	if (id == TBIDC("show_lsystem_panel") && _noiseSection != nullptr) {
		if (_lsystemSection->getVisibility() == tb::WIDGET_VISIBILITY_GONE) {
			_lsystemSection->setVisibility(tb::WIDGET_VISIBILITY_VISIBLE);
		} else {
			_lsystemSection->setVisibility(tb::WIDGET_VISIBILITY_GONE);
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
		return true;
	} else if (id == TBIDC("loadpalette")) {
		tb::TBPoint rootPos(ev.target_x, ev.target_y);
		ev.target->convertToRoot(rootPos.x, rootPos.y);
		PaletteSelector* selector = new PaletteSelector(this);
		selector->setPosition(rootPos);
		return true;
	} else if (id == TBIDC("scene_settings") && ev.ref_id == TBIDC("ok")) {
		auto &renderer = sceneMgr().renderer();
		if (_settings.ambientDirty) {
			const core::String& c = core::string::format("%f %f %f", _settings.ambientColor.x, _settings.ambientColor.y, _settings.ambientColor.z);
			core::Var::getSafe(cfg::VoxEditAmbientColor)->setVal(c);
		}
		if (_settings.diffuseDirty) {
			const core::String& c = core::string::format("%f %f %f", _settings.diffuseColor.x, _settings.diffuseColor.y, _settings.diffuseColor.z);
			core::Var::getSafe(cfg::VoxEditDiffuseColor)->setVal(c);
		}
		if (_settings.sunPositionDirty) {
			renderer.setSunPosition(_settings.sunPosition, glm::zero<glm::vec3>(), glm::up);
		}
		if (_settings.sunDirectionDirty) {
			// TODO: sun direction
		}
		return true;
	} else if (id == TBIDC("treegenerate")) {
		_treeGeneratorContext.cfg.pos = sceneMgr().referencePosition();
		uint8_t *basePtr = (uint8_t*)&_treeGeneratorContext;
		for (const TreeWidget& w : _treeWidgets) {
			if (w.treeType != voxelgenerator::TreeType::Max && w.treeType != _treeGeneratorContext.cfg.type) {
				continue;
			}
			uint8_t *targetPtr = basePtr + w.ctxOffset;
			if (w.type == TreeParameterWidgetType::Int) {
				*(int*)targetPtr = w.widget->getValue();
			} else if (w.type == TreeParameterWidgetType::Float) {
				*(float*)targetPtr = w.widget->getValueDouble();
			}
		}
		sceneMgr().createTree(_treeGeneratorContext);
		return true;
	} else if (id == TBIDC("scriptexecute")) {
		core::DynamicArray<core::String> activeScriptArgs;
		tb::TBLayout* treeParameterLayout = getWidgetByIDAndType<tb::TBLayout>("scriptparameterslayout");
		if (treeParameterLayout != nullptr) {
			tb::TBLinkListOf<TBWidget>::Iterator iter = treeParameterLayout->getIteratorForward();
			while (TBWidget* w = iter.getAndStep()) {
				if (tb::TBLayout* innerLayout = w->safeCastTo<tb::TBLayout>()) {
					if (tb::TBWidget* valueWidget = innerLayout->getLastChild()) {
						if (tb::TBEditField* stringField = valueWidget->safeCastTo<tb::TBEditField>()) {
							activeScriptArgs.push_back(stringField->getText());
						} else if (tb::TBInlineSelectDouble* floatField = valueWidget->safeCastTo<tb::TBInlineSelectDouble>()) {
							activeScriptArgs.push_back(core::string::toString(floatField->getValueDouble()));
						} else {
							activeScriptArgs.push_back(core::string::toString(valueWidget->getValue()));
						}
					}
				}
			}
		}

		sceneMgr().runScript(_activeScript, activeScriptArgs);
		return true;
	} else if (id == TBIDC("lsystemgenerate")) {
		const core::String& axiom = _lsystemAxiom->getText();
		const core::String& rulesStr = _lsystemRules->getText();
		core::DynamicArray<voxelgenerator::lsystem::Rule> rules;
		if (!voxelgenerator::lsystem::parseRules(rulesStr.c_str(), rules)) {
			Log::error("Failed to parse the lsystem rules");
			return true;
		}
		const float angle = _lsystemAngle->getValueDouble();
		const float length = _lsystemLength->getValueDouble();
		const float width = _lsystemWidth->getValueDouble();
		const float widthIncrement = _lsystemWidthIncrement->getValueDouble();
		const int iterations = _lsystemIterations->getValue();
		const float leavesRadius = _lsystemLeavesRadius->getValueDouble();
		sceneMgr().lsystem(axiom.c_str(), rules, angle, length, width, widthIncrement, iterations, leavesRadius);
		return true;
	} else if (id == TBIDC("noisegenerate")) {
		const int octaves = _octaves->getValue();
		const float frequency = _frequency->getValueDouble();
		const float lacunarity = _lacunarity->getValueDouble();
		const float gain = _gain->getValueDouble();
		sceneMgr().noise(octaves, lacunarity, frequency, gain, voxelgenerator::noise::NoiseType::ridgedMF);
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
	} else if (id == TBIDC("treetype")) {
		if (tb::TBSelectDropdown *dropdown = widget->safeCastTo<tb::TBSelectDropdown>()) {
			const int value = dropdown->getValue();
			switchTreeType(treeTypes[value].type);
			return true;
		}
		return false;
	} else if (id == TBIDC("scripttype")) {
		if (tb::TBSelectDropdown *dropdown = widget->safeCastTo<tb::TBSelectDropdown>()) {
			const int value = dropdown->getValue();
			if (value >= 0 && value < (int)_scripts.size()) {
				switchScriptType(_scripts[value]);
			}
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
		sceneMgr().animationEntity().setAnimation((animation::Animation)widget->getValue(), true);
		return true;
	} else if (id == TBIDC("optionvoxelsize")) {
		sceneMgr().setGridResolution(widget->getValue());
		return true;
	} else if (id == TBIDC("cursorx")) {
		const core::String& str = widget->getText();
		if (str.empty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = sceneMgr().cursorPosition();
		pos.x = val;
		sceneMgr().setCursorPosition(pos, true);
		return true;
	} else if (id == TBIDC("cursory")) {
		const core::String& str = widget->getText();
		if (str.empty()) {
			return true;
		}
		const int val = core::string::toInt(str);
		glm::ivec3 pos = sceneMgr().cursorPosition();
		pos.y = val;
		sceneMgr().setCursorPosition(pos, true);
		return true;
	} else if (id == TBIDC("cursorz")) {
		const core::String& str = widget->getText();
		if (str.empty()) {
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
			command::Command::execute("%s", *action);
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

	if (_treeAutoGenerateOnChange != nullptr && _treeAutoGenerateOnChange->getValue() == 1) {
		for (TreeWidget& w : _treeWidgets) {
			if (w.widget != widget) {
				continue;
			}
			if (ev.isAny(w.widget->getID())) {
				_treeGeneratorContext.cfg.pos = sceneMgr().referencePosition();
				uint8_t *basePtr = (uint8_t*)&_treeGeneratorContext;
				uint8_t *targetPtr = basePtr + w.ctxOffset;
				if (w.type == TreeParameterWidgetType::Int) {
					*(int*)targetPtr = w.widget->getValue();
				} else if (w.type == TreeParameterWidgetType::Float) {
					*(float*)targetPtr = w.widget->getValueDouble();
				}
				sceneMgr().createTree(_treeGeneratorContext);
				break;
			}
		}
	}

	return false;
}

void VoxEditWindow::onProcess() {
	Super::onProcess();

	const ModifierFacade& modifier = sceneMgr().modifier();
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
		_saveAnimationButton->setState(tb::WIDGET_STATE_DISABLED, sceneMgr().empty() || sceneMgr().editMode() != EditMode::Animation);
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
			if (SDL_strcmp(_cursorX->getText().c_str(), buf) != 0) {
				_cursorX->setText(buf);
			}
		}
		if (_cursorY != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.y);
			if (SDL_strcmp(_cursorY->getText().c_str(), buf) != 0) {
				_cursorY->setText(buf);
			}
		}
		if (_cursorZ != nullptr) {
			SDL_snprintf(buf, sizeof(buf), "%i", pos.z);
			if (SDL_strcmp(_cursorZ->getText().c_str(), buf) != 0) {
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
				command::Command::execute("%s", *action);
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

bool VoxEditWindow::save(const core::String& file) {
	if (file.empty()) {
		getApp()->saveDialog([this] (const core::String uifile) {save(uifile); }, voxelformat::SUPPORTED_VOXEL_FORMATS_SAVE);
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

bool VoxEditWindow::loadAnimationEntity(const core::String& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const core::String file) { core::String copy(file); loadAnimationEntity(copy); }, "lua");
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

bool VoxEditWindow::load(const core::String& file) {
	if (file.empty()) {
		getApp()->openDialog([this] (const core::String file) { core::String copy(file); load(copy); }, voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD);
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
