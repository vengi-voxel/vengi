/**
 * @file
 */

#include "VoxEdit.h"
#include "app/App.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "core/BindingContext.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "core/concurrent/Concurrency.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include "palette/PaletteFormatDescription.h"
#include "video/WindowedApp.h"
#include "voxedit-ui/QuitDisallowReason.h"
#include "voxedit-ui/Viewport.h"

#include "engine-git.h"
#include "voxedit-ui/MainWindow.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelui/FileDialogOptions.h"

VoxEdit::VoxEdit(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider,
				 const voxedit::SceneManagerPtr &sceneMgr, const voxelcollection::CollectionManagerPtr &collectionMgr,
				 const video::TexturePoolPtr &texturePool, const voxedit::SceneRendererPtr &sceneRenderer)
	: Super(filesystem, timeProvider, core::mostcpus()), _sceneMgr(sceneMgr), _sceneRenderer(sceneRenderer),
	  _collectionMgr(collectionMgr), _texturePool(texturePool), _paletteCache(sceneMgr, filesystem) {
	init(ORGANISATION, "voxedit");
	core::registerBindingContext("scene", core::BindingContext::Context1);
	core::registerBindingContext("model", core::BindingContext::Context2);
	core::registerBindingContext("editing", core::BindingContext::Context1 + core::BindingContext::Context2);
	_allowRelativeMouseMode = false;
	_iniVersion = 7;
	_keybindingsVersion = 1;
	_wantCrashLogs = true;

	// see KeyBindings enum
	_uiKeyMaps.push_back("Magicavoxel");
	_uiKeyMaps.push_back("Blender");
	_uiKeyMaps.push_back("Vengi");
	_uiKeyMaps.push_back("Qubicle");
	core_assert(KeyBindings::Max == (int)_uiKeyMaps.size());
}

void VoxEdit::printUsageHeader() const {
	Super::printUsageHeader();
	Log::info("Git commit " GIT_COMMIT " - " GIT_COMMIT_DATE);
}

app::AppState VoxEdit::onCleanup() {
	_sceneMgr->shutdown();
	if (_mainWindow) {
		_mainWindow->shutdown();
		delete _mainWindow;
	}
	_collectionMgr->shutdown();
	_texturePool->shutdown();
	return Super::onCleanup();
}

void VoxEdit::onDropFile(void *, const core::String &file) {
	if (_mainWindow == nullptr) {
		return;
	}
	if (_mainWindow->isPaletteWidgetDropTarget()) {
		if (_sceneMgr->importPalette(file, true, true)) {
			core::String paletteName(core::string::extractFilename(file));
			_mainWindow->onNewPaletteImport(paletteName, true, true);
			return;
		}
	}
	if (_sceneMgr->import(file)) {
		return;
	}
	Log::warn("Failed to handle %s as drop file event", file.c_str());
}

void VoxEdit::onDropText(void *, const core::String& text) {
	onDropFile(nullptr, text);
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

	for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
		_paletteFormats.push_back(*desc);
	}
	for (const io::FormatDescription *desc = voxelformat::voxelLoad(); desc->valid(); ++desc) {
		if (desc->flags & VOX_FORMAT_FLAG_PALETTE_EMBEDDED) {
			_paletteFormats.push_back(*desc);
		}
	}
	_paletteFormats.push_back(io::FormatDescription{"", {}, {}, 0u});

	_sceneMgr->construct();
	_collectionMgr->construct();
	_texturePool->construct();

	command::Command::registerCommand("screenshot", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		core::String viewportId = args.empty() ? "" : args[0];
		if (args.size() <= 1) {
			const core::String filename = _sceneMgr->getSuggestedFilename(io::format::png().mainExtension(false));
			saveDialog([this, viewportId] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->saveScreenshot(file, viewportId); }, voxelui::FileDialogOptions::build(_paletteCache, false), io::format::images(), filename);
			return;
		}
		_mainWindow->saveScreenshot(args[1], viewportId);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp(_("Save the current viewport as screenshot"));

	command::Command::registerCommand("togglescene", [this](const command::CmdArgs &args) {
		toggleScene();
	}).setHelp(_("Toggle scene mode on/off"));

	command::Command::registerCommand("save", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			const core::String filename = _sceneMgr->getSuggestedFilename();
			if (filename.empty()) {
				saveDialog([this] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->save(file, desc); }, voxelui::FileDialogOptions::build(_paletteCache, false), voxelformat::voxelSave(), "scene.vengi");
			} else {
				_mainWindow->save(filename, nullptr);
			}
			return;
		}
		_mainWindow->save(args[0], nullptr);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp(_("Save the current scene to the given file"));

	command::Command::registerCommand("saveas", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		const core::String &filename = _sceneMgr->getSuggestedFilename();
		saveDialog([this] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->save(file, desc); }, voxelui::FileDialogOptions::build(_paletteCache, false), voxelformat::voxelSave(), filename);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp(_("Save the current scene to the given file"));

	command::Command::registerCommand("exportselection", [&] (const command::CmdArgs& args) {
		static auto func = [this] (const core::String &file, const io::FormatDescription *desc) {
			io::FileDescription fd;
			fd.set(file, desc);
			_sceneMgr->saveSelection(fd);
		};
		saveDialog(func, voxelui::FileDialogOptions::build(_paletteCache, false), voxelformat::voxelSave());
	}).setHelp(_("Save the selection from the current active model node"));

	command::Command::registerCommand("load", [this](const command::CmdArgs &args) {
		if (_mainWindow == nullptr) {
			return;
		}
		if (args.empty()) {
			openDialog([this] (const core::String &file, const io::FormatDescription *desc) {_mainWindow->load(file, desc); }, voxelui::FileDialogOptions::build(_paletteCache, false), voxelformat::voxelLoad());
			return;
		}
		_mainWindow->load(args[0], nullptr);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp(_("Load a scene from the given volume file"));

	command::Command::registerCommand("import", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([this](const core::String &file, const io::FormatDescription *desc) { _sceneMgr->import(file); }, voxelui::FileDialogOptions::build(_paletteCache, false), voxelformat::voxelLoad());
			return;
		}
		_sceneMgr->import(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp(_("Add a volume to the existing scene from the given file"));

	command::Command::registerCommand("importdirectory", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			directoryDialog([this](const core::String &file, const io::FormatDescription *desc) { _sceneMgr->importDirectory(file); }, voxelui::FileDialogOptions::build(_paletteCache, false));
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
		_sceneMgr->importDirectory(args[0], format);
	}).setHelp(_("Import all files from a given directory"));

	command::Command::registerCommand("importpalette", [this](const command::CmdArgs &args) {
		if (args.empty()) {
			openDialog([this] (const core::String &file, const io::FormatDescription *desc) { importPalette(file); }, voxelui::FileDialogOptions::build(_paletteCache, true), &_paletteFormats[0]);
			return;
		}
		importPalette(args[0]);
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory, &_paletteFormats[0])).setHelp(_("Import an image as a palette"));

	command::Command::registerCommand("new", [this] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return false;
		}
		return _mainWindow->createNew(false);
	}).setHelp(_("Create a new scene with ui interaction"));

	command::Command::registerCommand("resetcamera", [this] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		_mainWindow->resetCamera();
	}).setHelp(_("Reset cameras in viewports"));

	return state;
}

void VoxEdit::importPalette(const core::String &file) {
	if (_sceneMgr->importPalette(file, false, false)) {
		core::String paletteName(core::string::extractFilename(file));
		_mainWindow->onNewPaletteImport(paletteName, false, false);
	} else {
		Log::error("Failed to execute 'importpalette' for file '%s'", file.c_str());
	}
}

void VoxEdit::loadKeymap(int keymap) {
	Super::loadKeymap(keymap);
	_keybindingHandler.registerBinding("ctrl+z",               "undo",                         "all");
	_keybindingHandler.registerBinding("shift+ctrl+z",         "redo",                         "all");
	_keybindingHandler.registerBinding("ctrl+y",               "redo",                         "all");
	_keybindingHandler.registerBinding("ctrl+o",               "load",                         "all");
	_keybindingHandler.registerBinding("ctrl+s",               "save",                         "all");
	_keybindingHandler.registerBinding("ctrl+shift+s",         "saveas",                       "all");
	_keybindingHandler.registerBinding("ctrl+n",               "new",                          "all");
	_keybindingHandler.registerBinding("tab",                  "togglescene",                  "editing");
	_keybindingHandler.registerBinding("delete",               "nodedelete",                   "scene");
	_keybindingHandler.registerBinding("shift+h",              "nodetogglevisible",            "editing");
	_keybindingHandler.registerBinding("shift+l",              "nodetogglelock",               "editing");
	_keybindingHandler.registerBinding("ctrl+c",               "copy",                         "editing");
	_keybindingHandler.registerBinding("h",                    "toggle ve_hideinactive",       "editing");
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
	_keybindingHandler.registerBinding("left_mouse",           "+actionexecute",               "model");
	_keybindingHandler.registerBinding("escape",               "abortaction",                  "model");
	_keybindingHandler.registerBinding("c",                    "pickcolor",                    "model");
	_keybindingHandler.registerBinding("return",               "setreferencepositiontocursor", "model");
	_keybindingHandler.registerBinding("keypad_enter",         "setreferencepositiontocursor", "model");
	_keybindingHandler.registerBinding("shift+r",              "setreferenceposition 0 0 0",   "model");
	_keybindingHandler.registerBinding("shift+d",              "actionerase",                  "model");
	_keybindingHandler.registerBinding("shift+p",              "actionplace",                  "model");
	_keybindingHandler.registerBinding("shift+o",              "actionoverride",               "model");
	_keybindingHandler.registerBinding("shift+x",              "lockx",                        "model");
	_keybindingHandler.registerBinding("shift+y",              "locky",                        "model");
	_keybindingHandler.registerBinding("shift+z",              "lockz",                        "model");
	_keybindingHandler.registerBinding("shift+1",              "lockx",                        "model");
	_keybindingHandler.registerBinding("shift+2",              "locky",                        "model");
	_keybindingHandler.registerBinding("shift+3",              "lockz",                        "model");
	_keybindingHandler.registerBinding("ctrl+1",               "mirroraxisshapebrushx",        "model");
	_keybindingHandler.registerBinding("ctrl+2",               "mirroraxisshapebrushy",        "model");
	_keybindingHandler.registerBinding("ctrl+3",               "mirroraxisshapebrushz",        "model");
	_keybindingHandler.registerBinding("ctrl+4",               "mirroraxisshapebrushnone",     "model");
	_keybindingHandler.registerBinding("wheelup",              "+zoom_in",                     "editing");
	_keybindingHandler.registerBinding("wheeldown",            "+zoom_out",                    "editing");
	_keybindingHandler.registerBinding("wheelleft",            "+zoom_in",                     "editing");
	_keybindingHandler.registerBinding("wheelright",           "+zoom_out",                    "editing");
	_keybindingHandler.registerBinding("ctrl+e",               "toggle r_renderoutline",       "all");
	_keybindingHandler.registerBinding("ctrl+g",               "toggle ve_showgrid",           "all");
	_keybindingHandler.registerBinding("ctrl+f",               "toggle ve_showaabb",           "all");
	_keybindingHandler.registerBinding("ctrl+w",               "toggle ve_rendershadow",       "all");
	_keybindingHandler.registerBinding("shift+c",              "brushpaint",                   "model");
	_keybindingHandler.registerBinding("l",                    "brushline",                    "model");
	_keybindingHandler.registerBinding("v",                    "brushshape",                   "model");
	_keybindingHandler.registerBinding("p",                    "brushstamp",                   "model");
	_keybindingHandler.registerBinding("f2",                   "toggle ve_popuprenamenode",    "all");
	_keybindingHandler.registerBinding("space",                "nodeduplicate",                "scene");
	_keybindingHandler.registerBinding("left_gui",             "+camera_pan",                  "editing");

	if (keymap != KeyBindings::Qubicle) {
		_keybindingHandler.registerBinding("w",                      "+move_forward",              "editing");
		_keybindingHandler.registerBinding("a",                      "+move_left",                 "editing");
		_keybindingHandler.registerBinding("s",                      "+move_backward",             "editing");
		_keybindingHandler.registerBinding("d",                      "+move_right",                "editing");
	}

	if (keymap == KeyBindings::Blender) {
		_keybindingHandler.registerBinding("ctrl+left_mouse",        "+actionexecutedelete",       "model");
		_keybindingHandler.registerBinding("1",                      "toggle ve_hideinactive",     "editing");
		_keybindingHandler.registerBinding("f5",                     "screenshot",                 "all");
		_keybindingHandler.registerBinding(",",                      "resetcamera",                "editing");
		_keybindingHandler.registerBinding("middle_mouse",           "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("alt+left_mouse",         "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("shift+middle_mouse",     "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("left_alt",               "+camera_pan",                "editing");
	} else if (keymap == KeyBindings::Magicavoxel) {
		_keybindingHandler.registerBinding("shift+left_mouse",       "+actionexecutedelete",       "model");
		_keybindingHandler.registerBinding("ctrl+shift+p",           "nodeduplicate",              "editing");
		_keybindingHandler.registerBinding("1",                      "mirroraxisshapebrushx",      "model");
		_keybindingHandler.registerBinding("2",                      "mirroraxisshapebrushz",      "model");
		_keybindingHandler.registerBinding("3",                      "mirroraxisshapebrushy",      "model");
		_keybindingHandler.registerBinding("4",                      "resetcamera",                "editing");
		_keybindingHandler.registerBinding("6",                      "screenshot",                 "editing");
		_keybindingHandler.registerBinding("f6",                     "screenshot",                 "all");
		_keybindingHandler.registerBinding("right_mouse",            "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("middle_mouse",           "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("alt+left_mouse",         "pickcolor",                  "model");
		_keybindingHandler.registerBinding("r",                      "actionerase",                "model");
		_keybindingHandler.registerBinding("g",                      "brushpaint",                 "model");
		_keybindingHandler.registerBinding("t",                      "actionplace",                "model");
		_keybindingHandler.registerBinding("n",                      "actionselect",               "model");
		_keybindingHandler.registerBinding("ctrl+h",                 "nodetogglevisible",          "editing");
	} else if (keymap == KeyBindings::Qubicle) {
		_keybindingHandler.registerBinding("ctrl+left_mouse",        "+actionexecutedelete",       "model");
		_keybindingHandler.registerBinding("1",                      "toggle ve_hideinactive",     "editing");
		_keybindingHandler.registerBinding("f5",                     "screenshot",                 "all");
		_keybindingHandler.registerBinding(",",                      "resetcamera",                "editing");
		_keybindingHandler.registerBinding("left_alt+left_mouse",    "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("right_alt+left_mouse",   "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("right_mouse",            "+camera_rotate",             "editing");
		_keybindingHandler.registerBinding("left_alt+middle_mouse",  "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("right_alt+middle_mouse", "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("middle_mouse",           "+camera_pan",                "editing");
		_keybindingHandler.registerBinding("i",                      "pickcolor",                  "model");
		_keybindingHandler.registerBinding("e",                      "actionerase",                "model");
		_keybindingHandler.registerBinding("a",                      "actionplace",                "model");
		_keybindingHandler.registerBinding("b",                      "brushpaint",                 "model");
		_keybindingHandler.registerBinding("m",                      "actionselect",               "model");
	} else /*if (keymap == KeyBindings::Vengi) */ {
		_keybindingHandler.registerBinding("ctrl+left_mouse",        "+actionexecutedelete",       "model");
		_keybindingHandler.registerBinding("1",                      "toggle ve_hideinactive",     "editing");
		_keybindingHandler.registerBinding("f5",                     "screenshot",                 "all");
		_keybindingHandler.registerBinding("e",                      "+actionexecute",             "model");
		_keybindingHandler.registerBinding(",",                      "resetcamera",                "editing");
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

	if (!_sceneMgr->init()) {
		Log::error("Failed to initialize the scene manager");
		return app::AppState::InitFailure;
	}

	if (!_texturePool->init()) {
		Log::error("Failed to initialize the texture pool");
		return app::AppState::InitFailure;
	}

	if (!_collectionMgr->init()) {
		Log::error("Failed to initialize the collection manager");
		return app::AppState::InitFailure;
	}

	_mainWindow = new voxedit::MainWindow(this, _sceneMgr, _texturePool, _collectionMgr, _filesystem, _paletteCache,
										  _sceneRenderer);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return app::AppState::InitFailure;
	}

	// needed for handling the module includes
	if (!filesystem()->registerPath("scripts/")) {
		Log::error("Failed to register lua generator script path");
		return app::AppState::InitFailure;
	}

	video::clearColor(::core::Color::Black());
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
		const io::FilePtr &filePtr = filesystem()->open(file);
		if (filePtr->exists()) {
			const core::String &filePath = filesystem()->sysAbsolutePath(filePtr->name());
			_mainWindow->load(filePath, nullptr);
		}
	} else {
		const core::String &file = loadingDocument();
		if (!file.empty()) {
			const io::FilePtr &filePtr = filesystem()->open(file);
			if (filePtr->exists()) {
				const core::String &filePath = filesystem()->sysAbsolutePath(filePtr->name());
				_mainWindow->load(filePath, nullptr);
			}
		}
	}

#ifdef IMGUI_ENABLE_TEST_ENGINE
	// register the ui tests late - as we need the main window
	_mainWindow->registerUITests(_imguiTestEngine, "###app");
#endif

	_paletteCache.detectPalettes();

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
	if (_sceneMgr->update(_nowSeconds)) {
		_mainWindow->onNewScene();
	}
	_mainWindow->update(_nowSeconds);
}

app::AppState VoxEdit::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}

	_collectionMgr->update(_nowSeconds);
	const voxedit::Viewport *viewport = _mainWindow->hoveredViewport();
	if (viewport) {
		if (viewport->isSceneMode()) {
			core::setBindingContext(core::BindingContext::Context1);
		} else {
			core::setBindingContext(core::BindingContext::Context2);
		}
	} else {
		core::setBindingContext(core::BindingContext::UI);
	}
	return state;
}
