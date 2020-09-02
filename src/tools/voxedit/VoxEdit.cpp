/**
 * @file
 */

#include "VoxEdit.h"
#include "app/App.h"
#include "core/Color.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/MaterialColor.h"
#include "metric/Metric.h"
#include "core/TimeProvider.h"
#include "core/EventBus.h"
#include "command/Command.h"
#include "command/CommandCompleter.h"
#include "video/Renderer.h"
#include "voxedit-ui/VoxEditWindow.h"
#include "io/Filesystem.h"
#include "voxedit-util/CustomBindingContext.h"

VoxEdit::VoxEdit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "voxedit");
	_allowRelativeMouseMode = false;
}

bool VoxEdit::importheightmapFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return false;
	}
	return _mainWindow->importHeightmap(file);
}

bool VoxEdit::importplaneFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return false;
	}
	return _mainWindow->importAsPlane(file);
}

bool VoxEdit::importpaletteFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return false;
	}
	return _mainWindow->importPalette(file);
}

bool VoxEdit::saveFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return false;
	}
	return _mainWindow->save(file);
}

bool VoxEdit::screenshotFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return false;
	}
	return _mainWindow->saveScreenshot(file);
}

bool VoxEdit::loadFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return false;
	}
	return _mainWindow->load(file);
}

bool VoxEdit::prefabFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return false;
	}
	return _mainWindow->prefab(file);
}

bool VoxEdit::newFile(bool force) {
	if (_mainWindow == nullptr) {
		return false;
	}
	return _mainWindow->createNew(force);
}

app::AppState VoxEdit::onCleanup() {
	voxedit::sceneMgr().shutdown();
	return Super::onCleanup();
}

void VoxEdit::onRenderUI() {
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

app::AppState VoxEdit::onConstruct() {
	const app::AppState state = Super::onConstruct();

	_framesPerSecondsCap->setVal(60.0f);

	voxedit::sceneMgr().construct();

#define COMMAND_FILE(commandId, help) \
	command::Command::registerCommand(#commandId, [this] (const command::CmdArgs& args) { \
		const core::String file = args.empty() ? "" : args[0]; \
		if (!commandId##File(file)) { \
			Log::error("Failed to execute '" #commandId "' for file '%s'", file.c_str()); \
		} \
	}).setArgumentCompleter(command::fileCompleter(io::filesystem(), _lastDirectory)).setHelp(help)

	COMMAND_FILE(screenshot, "Save the current viewport as screenshot");
	COMMAND_FILE(save, "Save the current scene as a volume to the given file");
	COMMAND_FILE(load, "Load a scene from the given volume file");
	COMMAND_FILE(prefab, "Add a volume to the existing scene from the given file");
	COMMAND_FILE(importheightmap, "Import a 2d heightmap image into the current active volume layer");
	COMMAND_FILE(importplane, "Import an image as a plane into a new layer");
	COMMAND_FILE(importpalette, "Import an image as a palette");
#undef COMMAND_FILE

	command::Command::registerCommand("animation_load", [&] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		const core::String file = args.empty() ? "" : args[0];
		_mainWindow->loadAnimationEntity(file);
	}).setHelp("Load the animation volumes and settings").setArgumentCompleter(command::fileCompleter(io::filesystem(), "", "*.lua"));

	command::Command::registerCommand("new", [this] (const command::CmdArgs& args) {
		newFile();
	}).setHelp("Create a new scene with ui interaction");

	command::Command::registerCommand("toggleviewport", [this] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		_mainWindow->toggleViewport();
	}).setHelp("Toggle quad view on/off");

	command::Command::registerCommand("toggleanimation", [this] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		_mainWindow->toggleAnimation();
	}).setHelp("Toggle animation view on/off");

	command::Command::registerCommand("resetcamera", [this] (const command::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		_mainWindow->resetCamera();
	}).setHelp("Reset cameras in all viewports");

	core::Var::get(cfg::RenderOutline, "false", core::CV_SHADER, "Render the scene with voxel outlines");

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

	_mainWindow = new voxedit::VoxEditWindow(this);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return app::AppState::InitFailure;
	}

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

	core::setBindingContext(voxedit::BindingContext::UI);

	if (_argc >= 2) {
		const char *file = _argv[_argc - 1];
		const io::FilePtr& filePtr = filesystem()->open(file);
		if (filePtr->exists()) {
			voxedit::sceneMgr().load(filePtr->name());
		}
	}

	return state;
}

app::AppState VoxEdit::onRunning() {
	app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}
	voxedit::sceneMgr().update(_nowSeconds);
	_mainWindow->update();
	const bool isSceneHovered = _mainWindow->isSceneHovered();
	if (isSceneHovered) {
		if (voxedit::sceneMgr().editMode() == voxedit::EditMode::Scene) {
			core::setBindingContext(voxedit::BindingContext::Scene);
		} else {
			core::setBindingContext(voxedit::BindingContext::Model);
		}
	} else {
		core::setBindingContext(voxedit::BindingContext::UI);
	}
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	VoxEdit app(metric, filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
