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
#include "voxedit-ui/QuitDisallowReason.h"
#include "voxedit-ui/Viewport.h"
#include "voxedit-util/Config.h"
#include "core/TimeProvider.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "video/Renderer.h"
#include "io/Filesystem.h"
#include "core/Log.h"

#include "voxedit-util/SceneManager.h"

#include "voxedit-ui/MainWindow.h"
#include "voxedit-ui/FileDialogOptions.h"
#include "voxelformat/FormatConfig.h"
#include "voxelformat/VolumeFormat.h"
#include "core/StandardLib.h"

VoxEdit::VoxEdit(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider, core::halfcpus()) {
	init(ORGANISATION, "voxedit");
	core::registerBindingContext("scene", core::BindingContext::Context1);
	core::registerBindingContext("model", core::BindingContext::Context2);
	core::registerBindingContext("editing", core::BindingContext::Context1 + core::BindingContext::Context2);
	_allowRelativeMouseMode = false;
	_iniVersion = 1;
	_keybindingsVersion = 1;

	// see KeyBindings enum
	_uiKeyMaps.push_back("Magicavoxel");
	_uiKeyMaps.push_back("Blender");
	_uiKeyMaps.push_back("Vengi");
	_uiKeyMaps.push_back("Qubicle");
	core_assert(KeyBindings::Max == (int)_uiKeyMaps.size());
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
	if (voxedit::sceneMgr().import(file)) {
		return;
	}
	Log::warn("Failed to handle %s as drop file event", file.c_str());
}

core::String VoxEdit::getSuggestedFilename(const char *extension) const {
	core::String filename = voxedit::sceneMgr().filename();
	if (filename.empty()) {
		if (extension != nullptr && !SDL_strcasecmp(extension, "vengi")) {
			return "scene.vengi";
		}
		return filename;
	}
	if (extension == nullptr) {
		return filename;
	}
	return core::string::replaceExtension(filename, extension);
}

void VoxEdit::toggleScene() {
	if (_mainWindow == nullptr) {
		return;
	}
	_mainWindow->toggleScene();
}

app::AppState VoxEdit::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_framesPerSecondsCap->setVal(60.0f);

	core::Var::get(cfg::VoxEditColorWheel, "false", "Use the color wheel in the palette color editing", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowColorPicker, "false", "Always show the color picker below the palette", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditModificationDismissMillis, "1500", "Milliseconds that a region should get highlighted in a few situations");
	core::Var::get(cfg::VoxEditModificationDismissMillis, "1500", "Milliseconds that a region should get highlighted in a few situations");
	core::Var::get(cfg::VoxEditRegionSizes, "", "Show fixed region sizes in the positions panel", core::Var::ivec3ListValidator<1, 256>);
	core::Var::get(cfg::VoxEditShowgrid, "true", "Show the grid", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowlockedaxis, "true", "Show the currently locked axis", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditShowaabb, "true", "Show the axis aligned bounding box", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditRendershadow, "true", "Render with shadows - make sure to set the scene lighting up properly", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditAnimationSpeed, "100", "Millisecond delay between frames hide/unhide when using the scene graph panel play button to animate the models in the scene");
	core::Var::get(cfg::VoxEditGridsize, "1", "The size of the voxel grid", core::Var::minMaxValidator<1, 64>);
	core::Var::get(cfg::VoxEditLastFile, "");
	core::Var::get(cfg::VoxEditLastFiles, "");
	core::Var::get(cfg::VoxEditGrayInactive, "false", "Render the inactive nodes in gray scale mode", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditHideInactive, "false", "Hide the inactive nodes", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditViewdistance, "5000");
	core::Var::get(cfg::VoxEditShowaxis, "true", "Show the axis", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditCursorDetails, "1", "Print cursor details in edit mode", core::Var::minMaxValidator<0, 2>);
	core::Var::get(cfg::VoxEditGizmoRotation, "false", "Activate rotations for the gizmo in scene mode", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGizmoAllowAxisFlip, "true", "Flip axis or stay along the positive world/local axis", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGizmoSnap, "true", "Use the grid size for snap", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditGizmoBounds, "false", "Allow to change the volume bounds in scene mode", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditModelGizmo, "false", "Show the gizmo to also translate the region", core::Var::boolValidator);
	core::Var::get(cfg::VoxEditLastPalette, voxel::Palette::builtIn[0]);
	core::Var::get(cfg::VoxEditViewports, "2", "The amount of viewports (not in simple ui mode)", core::Var::minMaxValidator<2, cfg::MaxViewports>);
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
		core::String viewportId = args.empty() ? "" : args[0];
		if (args.size() <= 1) {
			const core::String filename = getSuggestedFilename("png");
			saveDialog([this, viewportId] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->saveScreenshot(file, viewportId); }, fileDialogOptions, io::format::images(), filename);
			return;
		}
		_mainWindow->saveScreenshot(args[1], viewportId);
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
				saveDialog([this] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->save(file, desc); }, fileDialogOptions, voxelformat::voxelSave(), "scene.vengi");
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
		const core::String &filename = getSuggestedFilename("vengi");
		saveDialog([this] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->save(file, desc); }, fileDialogOptions, voxelformat::voxelSave(), filename);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Save the current scene as a volume to the given file");

	command::Command::registerCommand("exportselection", [&] (const command::CmdArgs& args) {
		static auto func = [] (const core::String &file, const io::FormatDescription *desc) {
			io::FileDescription fd;
			fd.set(file, desc);
			voxedit::sceneMgr().saveSelection(fd);
		};
		saveDialog(func, fileDialogOptions, voxelformat::voxelSave());
	}).setHelp("Save the selection from the current active model node");

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

	command::Command::registerCommand("import", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([](const core::String &file, const io::FormatDescription *desc) { voxedit::sceneMgr().import(file); }, fileDialogOptions, voxelformat::voxelLoad());
			return;
		}
		voxedit::sceneMgr().import(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Add a volume to the existing scene from the given file");

	command::Command::registerCommand("importdirectory", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			directoryDialog([](const core::String &file, const io::FormatDescription *desc) { voxedit::sceneMgr().importDirectory(file); }, fileDialogOptions);
			return;
		}
		const io::FormatDescription* format = nullptr;
		if (args.size() == 2) {
			for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
				if (desc->matchesExtension(args[1])) {
					format = desc;
					break;
				}
			}
			if (format == nullptr) {
				Log::error("Could not find a supported format for %s", args[1].c_str());
				return;
			}
		}
		voxedit::sceneMgr().importDirectory(args[0], format);
	}).setHelp("Import all files from a given directory");

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
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp("Import a 2d heightmap image into the current active node. The height is encoded in the alpha channel with this method.");

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
	}).setHelp("Reset cameras in viewports");

	return state;
}

void VoxEdit::loadKeymap(int keymap) {
	_keybindingHandler.registerBinding("ctrl+z",               "undo",                         "all");
	_keybindingHandler.registerBinding("shift+ctrl+z",         "redo",                         "all");
	_keybindingHandler.registerBinding("ctrl+y",               "redo",                         "all");
	_keybindingHandler.registerBinding("ctrl+o",               "load",                         "all");
	_keybindingHandler.registerBinding("ctrl+s",               "save",                         "all");
	_keybindingHandler.registerBinding("ctrl+shift+s",         "saveas",                       "all");
	_keybindingHandler.registerBinding("ctrl+n",               "new",                          "all");
	_keybindingHandler.registerBinding("tab",                  "togglescene",                  "editing");
	_keybindingHandler.registerBinding("delete",               "layerdelete",                  "scene");
	_keybindingHandler.registerBinding("shift+h",              "togglelayerstate",             "editing");
	_keybindingHandler.registerBinding("shift+l",              "togglelayerlock",              "editing");
	_keybindingHandler.registerBinding("ctrl+c",               "copy",                         "editing");
	_keybindingHandler.registerBinding("ctrl+v",               "paste",                        "editing");
	_keybindingHandler.registerBinding("ctrl+x",               "cut",                          "editing");
	_keybindingHandler.registerBinding("ctrl+shift+v",         "pastecursor",                  "editing");
	_keybindingHandler.registerBinding("double_left_mouse",    "mouse_node_select",            "scene");
	_keybindingHandler.registerBinding("ctrl+a",               "select all",                   "model");
	_keybindingHandler.registerBinding("ctrl+d",               "select none",                  "model");
	_keybindingHandler.registerBinding("ctrl+i",               "select invert",                "model");
	_keybindingHandler.registerBinding("+",                    "resize 1",                     "model");
	_keybindingHandler.registerBinding("-",                    "resize -1",                    "model");
	_keybindingHandler.registerBinding("left",                 "+movecursorleft",              "model");
	_keybindingHandler.registerBinding("right",                "+movecursorright",             "model");
	_keybindingHandler.registerBinding("up",                   "+movecursorforward",           "model");
	_keybindingHandler.registerBinding("down",                 "+movecursorbackward",          "model");
	_keybindingHandler.registerBinding("ctrl+up",              "+movecursorup",                "model");
	_keybindingHandler.registerBinding("ctrl+down",            "+movecursordown",              "model");
	_keybindingHandler.registerBinding("v",                    "+actionexecute",               "model");
	_keybindingHandler.registerBinding("left_mouse",           "+actionexecute",               "model");
	_keybindingHandler.registerBinding("escape",               "abortaction",                  "model");
	_keybindingHandler.registerBinding("p",                    "pickcolor",                    "model");
	_keybindingHandler.registerBinding("return",               "setreferencepositiontocursor", "model");
	_keybindingHandler.registerBinding("keypad_enter",         "setreferencepositiontocursor", "model");
	_keybindingHandler.registerBinding("shift+r",              "setreferenceposition 0 0 0",   "model");
	_keybindingHandler.registerBinding("shift+d",              "actionerase",                  "model");
	_keybindingHandler.registerBinding("shift+c",              "actionpaint",                  "model");
	_keybindingHandler.registerBinding("shift+p",              "actionplace",                  "model");
	_keybindingHandler.registerBinding("shift+o",              "actionoverride",               "model");
	_keybindingHandler.registerBinding("shift+x",              "lockx",                        "model");
	_keybindingHandler.registerBinding("shift+y",              "locky",                        "model");
	_keybindingHandler.registerBinding("shift+z",              "lockz",                        "model");
	_keybindingHandler.registerBinding("shift+1",              "lockx",                        "model");
	_keybindingHandler.registerBinding("shift+2",              "locky",                        "model");
	_keybindingHandler.registerBinding("shift+3",              "lockz",                        "model");
	_keybindingHandler.registerBinding("ctrl+1",               "mirroraxisx",                  "model");
	_keybindingHandler.registerBinding("ctrl+2",               "mirroraxisy",                  "model");
	_keybindingHandler.registerBinding("ctrl+3",               "mirroraxisz",                  "model");
	_keybindingHandler.registerBinding("ctrl+4",               "mirroraxisnone",               "model");
	_keybindingHandler.registerBinding("wheelup",              "+zoom_in",                     "editing");
	_keybindingHandler.registerBinding("wheeldown",            "+zoom_out",                    "editing");
	_keybindingHandler.registerBinding("ctrl+e",               "toggle r_renderoutline",       "all");
	_keybindingHandler.registerBinding("ctrl+g",               "toggle ve_showgrid",           "all");
	_keybindingHandler.registerBinding("ctrl+f",               "toggle ve_showaabb",           "all");
	_keybindingHandler.registerBinding("ctrl+w",               "toggle ve_rendershadow",       "all");
	_keybindingHandler.registerBinding("w",                    "+move_forward",                "editing");
	_keybindingHandler.registerBinding("a",                    "+move_left",                   "editing");
	_keybindingHandler.registerBinding("s",                    "+move_backward",               "editing");
	_keybindingHandler.registerBinding("d",                    "+move_right",                  "editing");

	if (keymap == KeyBindings::Blender) {
		_keybindingHandler.registerBinding("ctrl+left_mouse",        "+actionexecutedelete",       "model");
		_keybindingHandler.registerBinding("1",                      "toggle ve_hideinactive",     "editing");
		_keybindingHandler.registerBinding("space",                  "nodeduplicate",              "editing");
		_keybindingHandler.registerBinding("f5",                     "screenshot",                 "all");
		_keybindingHandler.registerBinding(",",                      "resetcamera",                "editing");
		_keybindingHandler.registerBinding("ctrl+tab",               "toggleconsole",              "all");
		_keybindingHandler.registerBinding("middle_mouse",           "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("alt+left_mouse",         "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("shift+middle_mouse",     "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("left_alt",               "+camera_pan",                "editing");
	} else if (keymap == KeyBindings::Magicavoxel) {
		_keybindingHandler.registerBinding("shift+left_mouse",       "+actionexecutedelete",       "model");
		_keybindingHandler.registerBinding("ctrl+shift+p",           "nodeduplicate",              "editing");
		_keybindingHandler.registerBinding("1",                      "mirroraxisx",                "model");
		_keybindingHandler.registerBinding("2",                      "mirroraxisz",                "model");
		_keybindingHandler.registerBinding("3",                      "mirroraxisy",                "model");
		_keybindingHandler.registerBinding("4",                      "resetcamera",                "editing");
		_keybindingHandler.registerBinding("6",                      "screenshot",                 "all");
		_keybindingHandler.registerBinding("f1",                     "toggleconsole",              "all");
		_keybindingHandler.registerBinding("f6",                     "screenshot",                 "all");
		_keybindingHandler.registerBinding("right_mouse",            "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("middle_mouse",           "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("alt+left_mouse",         "pickcolor",                  "model");
		_keybindingHandler.registerBinding("r",                      "actionerase",                "model");
		_keybindingHandler.registerBinding("g",                      "actionpaint",                "model");
		_keybindingHandler.registerBinding("t",                      "actionplace",                "model");
	} else if (keymap == KeyBindings::Qubicle) {
		_keybindingHandler.registerBinding("ctrl+left_mouse",        "+actionexecutedelete",       "model");
		_keybindingHandler.registerBinding("1",                      "toggle ve_hideinactive",     "editing");
		_keybindingHandler.registerBinding("space",                  "nodeduplicate",              "editing");
		_keybindingHandler.registerBinding("f5",                     "screenshot",                 "all");
		_keybindingHandler.registerBinding(",",                      "resetcamera",                "editing");
		_keybindingHandler.registerBinding("ctrl+tab",               "toggleconsole",              "all");
		_keybindingHandler.registerBinding("left_alt+left_mouse",    "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("right_alt+left_mouse",   "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("right_mouse",            "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("left_alt+middle_mouse",  "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("right_alt+middle_mouse", "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("middle_mouse",           "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("i",                      "pickcolor",                  "model");
		_keybindingHandler.registerBinding("e",                      "actionerase",                "model");
		_keybindingHandler.registerBinding("a",                      "actionplace",                "model");
		_keybindingHandler.registerBinding("b",                      "actionpaint",                "model");
		_keybindingHandler.registerBinding("m",                      "actionselect",               "model");
	} else /*if (keymap == KeyBindings::Vengi) */ {
		_keybindingHandler.registerBinding("ctrl+left_mouse",        "+actionexecutedelete",       "model");
		_keybindingHandler.registerBinding("1",                      "toggle ve_hideinactive",     "editing");
		_keybindingHandler.registerBinding("space",                  "nodeduplicate",              "editing");
		_keybindingHandler.registerBinding("f5",                     "screenshot",                 "all");
		_keybindingHandler.registerBinding(",",                      "resetcamera",                "editing");
		_keybindingHandler.registerBinding("ctrl+tab",               "toggleconsole",              "all");
		_keybindingHandler.registerBinding("right_mouse",            "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("middle_mouse",           "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("left_alt",               "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("right_alt",              "+camera_pan",                "editing");
	}
}

app::AppState VoxEdit::onInit() {
	const app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (_keybindingHandler.bindings().empty()) {
		loadKeymap(_uiKeyMap->intVal());
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
	voxedit::QuitDisallowReason reason = _mainWindow->allowToQuit();
	if (reason == voxedit::QuitDisallowReason::UnsavedChanges) {
		_showFileDialog = false;
		return false;
	}
	return true;
}

void VoxEdit::onRenderUI() {
	if (voxedit::sceneMgr().update(_nowSeconds)) {
		_mainWindow->onNewScene();
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
	const core::TimeProviderPtr& timeProvider = core::make_shared<core::TimeProvider>();
	VoxEdit app(filesystem, timeProvider);
	return app.startMainLoop(argc, argv);
}
