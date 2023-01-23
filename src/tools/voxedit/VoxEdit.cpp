/**
 * @file
 */

#include "VoxEdit.h"
#include "app/App.h"
#include "core/BindingContext.h"
#include "core/Color.h"
#include "core/GameConfig.h"
#include "core/StringUtil.h"
#include "core/concurrent/Concurrency.h"
#include "io/FormatDescription.h"
#include "video/WindowedApp.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/Config.h"
#include "core/TimeProvider.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "video/Renderer.h"
#include "io/Filesystem.h"

#include "voxedit-util/SceneManager.h"

#include "voxedit-ui/MainWindow.h"
#include "voxedit-ui/FileDialogOptions.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"

VoxEdit::VoxEdit(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider, core::halfcpus()) {
	init(ORGANISATION, "voxedit");
	core::registerBindingContext("scene", core::BindingContext::Context1);
	core::registerBindingContext("model", core::BindingContext::Context2);
	core::registerBindingContext("editing", core::BindingContext::Context1 + core::BindingContext::Context2);
	_allowRelativeMouseMode = false;
	_iniVersion = 1;
	_keybindingsVersion = 0;
}

app::AppState VoxEdit::onCleanup() {
	voxedit::sceneMgr().shutdown();
	if (_mainWindow) {
		_mainWindow->shutdown();
		delete _mainWindow;
	}
	return Super::onCleanup();
}

void VoxEdit::onDropFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return;
	}
	if (_mainWindow->isPaletteWidgetDropTarget()) {
		if (voxedit::sceneMgr().importPalette(file)) {
			return;
		}
	}
	if (voxedit::sceneMgr().prefab(file)) {
		return;
	}
	Log::warn("Failed to handle %s as drop file event", file.c_str());
}

core::String VoxEdit::getSuggestedFilename(const char *extension) const {
	core::String filename = voxedit::sceneMgr().filename();
	if (filename.empty()) {
		return filename;
	}
	if (extension == nullptr) {
		return filename;
	}
	return core::string::stripExtension(filename) + "." + extension;
}

static bool ivec3ListValidator(const core::String& value) {
	if (value.empty()) {
		return true;
	}
	core::DynamicArray<core::String> regionSizes;
	core::string::splitString(value, regionSizes, ",");
	for (const core::String &s : regionSizes) {
		const glm::ivec3 maxs = core::string::parseIVec3(s);
		for (int i = 0; i < 3; ++i) {
			if (maxs[i] <= 0 || maxs[i] > 256) {
				return false;
			}
		}
	}
	return true;
}

void VoxEdit::toggleScene() {
	if (_mainWindow == nullptr) {
		return;
	}
	_mainWindow->toggleScene();
}

app::AppState VoxEdit::onConstruct() {
	core::Var::get(cfg::ClientCameraMaxZoom, "1000.0");
	core::Var::get(cfg::ClientCameraMinZoom, "0.1");
	core::Var::get(cfg::ClientFullscreen, "false", "Fullscreen or window", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditColorWheel, "false", "Use the color wheel in the palette color editing", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowColorPicker, "false", "Always show the color picker below the palette", core::Var::boolValidator);
	const app::AppState state = Super::onConstruct();
	_framesPerSecondsCap->setVal(60.0f);

	core::Var::get(cfg::VoxEditRegionSizes, "", "Show fixed region sizes in the positions panel", ivec3ListValidator);
	core::Var::get(cfg::VoxEditShowgrid, "true", "Show the grid", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowlockedaxis, "true", "Show the currently locked axis", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowaabb, "true", "Show the axis aligned bounding box", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditRendershadow, "true", "Render with shadows", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditAnimationSpeed, "100", "Millisecond delay between frames hide/unhide when using the scene graph panel play button to animate the models in the scene");
	core::Var::get(cfg::VoxEditGridsize, "1", "The size of the voxel grid", [](const core::String &val) {
		const int intVal = core::string::toInt(val);
		return intVal >= 1 && intVal <= 64;
	});
	core::Var::get(cfg::VoxEditLastFile, "");
	core::Var::get(cfg::VoxEditLastFiles, "");
	core::Var::get(cfg::VoxEditGrayInactive, "false", "Render the inactive nodes in gray scale mode", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditHideInactive, "false", "Hide the inactive nodes", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditViewdistance, "5000");
	core::Var::get(cfg::VoxEditShowaxis, "true", "Show the axis", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGuizmoRotation, "false", "Activate rotations for the guizmo in scene mode", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGuizmoAllowAxisFlip, "true", "Flip axis or stay along the positive world/local axis", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGuizmoSnap, "true", "Use the grid size for snap", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditLastPalette, voxel::Palette::builtIn[0]);
	core::Var::get(cfg::VoxEditViewports, "2", "The amount of viewports (not in simple ui mode)", [](const core::String &val) {
		const int intVal = core::string::toInt(val);
		return intVal >= 2 && intVal <= cfg::MaxViewports;
	});
	core::Var::get(cfg::VoxEditSimplifiedView, "false", "Hide some panels to simplify the ui - restart on change", core::Var::boolValidator);

	voxelformat::FormatConfig::init();

	for (const io::FormatDescription* desc = io::format::palettes(); desc->valid(); ++desc) {
		_paletteFormats.push_back(*desc);
	}
	for (const io::FormatDescription* desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		if (desc->flags & VOX_FORMAT_FLAG_PALETTE_EMBEDDED) {
			_paletteFormats.push_back(*desc);
		}
	}
	_paletteFormats.push_back(io::FormatDescription{"", {}, nullptr, 0u});

	voxedit::sceneMgr().construct();

	command::Command::registerCommand("screenshot", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			const core::String filename = getSuggestedFilename("png");
			saveDialog([this] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->saveScreenshot(file); }, fileDialogOptions, io::format::images(), filename);
			return;
		}
		_mainWindow->saveScreenshot(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Save the current viewport as screenshot");

	command::Command::registerCommand("togglescene", [this](const command::CmdArgs &args) {
		toggleScene();
	}).setHelp("Toggle scene mode on/off");

	command::Command::registerCommand("save", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			const core::String filename = getSuggestedFilename();
			if (filename.empty()) {
				saveDialog([this] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->save(file, desc); }, fileDialogOptions, voxelformat::voxelSave());
			} else {
				_mainWindow->save(filename, nullptr);
			}
			return;
		}
		_mainWindow->save(args[0], nullptr);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Save the current scene as a volume to the given file");

	command::Command::registerCommand("saveas", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		const core::String filename = getSuggestedFilename();
		saveDialog([this] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->save(file, desc); }, fileDialogOptions, voxelformat::voxelSave(), filename);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Save the current scene as a volume to the given file");

	command::Command::registerCommand("load", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			openDialog([this] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->load(file, desc); }, fileDialogOptions, voxelformat::voxelLoad());
			return;
		}
		_mainWindow->load(args[0], nullptr);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Load a scene from the given volume file");

	command::Command::registerCommand("prefab", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([](const core::String &file, const io::FormatDescription *desc) { voxedit::sceneMgr().prefab(file); }, fileDialogOptions, voxelformat::voxelLoad());
			return;
		}
		voxedit::sceneMgr().prefab(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Add a volume to the existing scene from the given file");

	command::Command::registerCommand("importheightmap", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file, const io::FormatDescription *desc) { voxedit::sceneMgr().importHeightmap(file); }, fileDialogOptions, io::format::images());
			return;
		}
		if (!voxedit::sceneMgr().importHeightmap(args[0])) {
			Log::error("Failed to execute 'importheightmap' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import a 2d heightmap image into the current active node");

	command::Command::registerCommand("importcoloredheightmap", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file, const io::FormatDescription *desc) { voxedit::sceneMgr().importHeightmap(file); }, fileDialogOptions, io::format::images());
			return;
		}
		if (!voxedit::sceneMgr().importColoredHeightmap(args[0])) {
			Log::error("Failed to execute 'importcoloredheightmap' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import a 2d heightmap image into the current active node");

	command::Command::registerCommand("importplane", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file, const io::FormatDescription *desc) { voxedit::sceneMgr().importAsPlane(file); }, fileDialogOptions, io::format::images());
			return;
		}
		if (!voxedit::sceneMgr().importAsPlane(args[0])) {
			Log::error("Failed to execute 'importplane' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import an image as a plane into a new node");

	command::Command::registerCommand("importvolume", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file, const io::FormatDescription *desc) { voxedit::sceneMgr().importAsVolume(file, 8, true); }, fileDialogOptions, io::format::images());
			return;
		}
		const int maxDepth = args.size() >= 2 ? core::string::toInt(args[1]) : 8;
		const bool bothSides = args.size() >= 3 ? core::string::toBool(args[2]) : true;
		if (!voxedit::sceneMgr().importAsVolume(args[0], maxDepth, bothSides)) {
			Log::error("Failed to execute 'importvolume' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import an image as a volume into a new node");

	command::Command::registerCommand("importpalette", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([] (const core::String &file, const io::FormatDescription *desc) { voxedit::sceneMgr().importPalette(file); }, fileDialogOptions, &_paletteFormats[0]);
			return;
		}
		if (!voxedit::sceneMgr().importPalette(args[0])) {
			Log::error("Failed to execute 'importpalette' for file '%s'", args[0].c_str());
		}
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory, &_paletteFormats[0])).setHelp("Import an image as a palette");

	command::Command::registerCommand("new", [this] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return false;
		}
		return _mainWindow->createNew(false);
	}).setHelp("Create a new scene with ui interaction");

	command::Command::registerCommand("resetcamera", [this] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		_mainWindow->resetCamera();
	}).setHelp("Reset cameras in all viewports");

	return state;
}

app::AppState VoxEdit::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!voxedit::sceneMgr().init()) {
		Log::error("Failed to initialize the scene manager");
		return app::AppState::InitFailure;
	}

	_mainWindow = new voxedit::MainWindow(this);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return app::AppState::InitFailure;
	}

	// needed for handling the module includes
	if (!filesystem()->registerPath("scripts/")) {
		Log::error("Failed to register lua generator script path");
		return app::AppState::InitFailure;
	}

	video::clearColor(::core::Color::Black);
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);
	video::enable(video::State::Blend);
	video::blendFunc(video::BlendMode::SourceAlpha, video::BlendMode::OneMinusSourceAlpha);

	setRelativeMouseMode(false);

	core::setBindingContext(core::BindingContext::UI);

	if (_argc >= 2) {
		const char *file = _argv[_argc - 1];
		const io::FilePtr& filePtr = filesystem()->open(file);
		if (filePtr->exists()) {
			const core::String &filePath = filesystem()->absolutePath(filePtr->name());
			_mainWindow->load(filePath, nullptr);
		}
	}

	return state;
}

bool VoxEdit::allowedToQuit() {
	return _mainWindow->allowToQuit();
}

void VoxEdit::onRenderUI() {
	if (voxedit::sceneMgr().update(_nowSeconds)) {
		_mainWindow->resetCamera();
	}
	_mainWindow->update();
}

app::AppState VoxEdit::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}
	const voxedit::Viewport *scene = _mainWindow->hoveredScene();
	if (scene) {
		if (scene->isSceneMode()) {
			core::setBindingContext(core::BindingContext::Context1);
		} else {
			core::setBindingContext(core::BindingContext::Context2);
		}
	} else {
		core::setBindingContext(core::BindingContext::UI);
	}
	return state;
}

int main(int argc, char *argv[]) {
	const io::FilesystemPtr& filesystem = core::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	VoxEdit app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
