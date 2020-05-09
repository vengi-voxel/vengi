/**
 * @file
 */

#include "VoxEdit.h"
#include "core/Color.h"
#include "voxel/MaterialColor.h"
#include "core/metric/Metric.h"
#include "core/TimeProvider.h"
#include "core/EventBus.h"
#include "core/command/Command.h"
#include "core/command/CommandCompleter.h"
#include "video/Renderer.h"
#include "ui/VoxEditWindow.h"
#include "core/io/Filesystem.h"
#include "voxedit-util/CustomBindingContext.h"

VoxEdit::VoxEdit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _mainWindow(nullptr), _sceneMgr(voxedit::sceneMgr()) {
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

core::AppState VoxEdit::onCleanup() {
	_sceneMgr.shutdown();
	return Super::onCleanup();
}

void VoxEdit::onDropFile(const core::String& file) {
	if (_mainWindow == nullptr) {
		return;
	}
	if (_mainWindow->isPaletteWidgetDropTarget()) {
		if (_sceneMgr.importPalette(file)) {
			return;
		}
	}
	if (_sceneMgr.prefab(file)) {
		return;
	}
	Log::warn("Failed to handle %s as drop file event", file.c_str());
}

core::AppState VoxEdit::onConstruct() {
	const core::AppState state = Super::onConstruct();

	_framesPerSecondsCap->setVal(60.0f);

	_sceneMgr.construct();

#define COMMAND_FILE(command, help) \
	core::Command::registerCommand(#command, [this] (const core::CmdArgs& args) { \
		const core::String file = args.empty() ? "" : args[0]; \
		if (!command##File(file)) { \
			Log::error("Failed to execute '" #command "' for file '%s'", file.c_str()); \
		} \
	}).setArgumentCompleter(core::fileCompleter(_lastDirectory)).setHelp(help)

	COMMAND_FILE(screenshot, "Save the current viewport as screenshot");
	COMMAND_FILE(save, "Save the current scene as a volume to the given file");
	COMMAND_FILE(load, "Load a scene from the given volume file");
	COMMAND_FILE(prefab, "Add a volume to the existing scene from the given file");
	COMMAND_FILE(importheightmap, "Import a 2d heightmap image into the current active volume layer");
	COMMAND_FILE(importplane, "Import an image as a plane into a new layer");
	COMMAND_FILE(importpalette, "Import an image as a palette");
#undef COMMAND_FILE

	core::Command::registerCommand("animation_load", [&] (const core::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		const core::String file = args.empty() ? "" : args[0];
		_mainWindow->loadAnimationEntity(file);
	}).setHelp("Load the animation volumes and settings").setArgumentCompleter(core::fileCompleter("", "*.lua"));

	core::Command::registerCommand("new", [this] (const core::CmdArgs& args) {
		newFile();
	}).setHelp("Create a new scene with ui interaction");

	core::Command::registerCommand("toggleviewport", [this] (const core::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		_mainWindow->toggleViewport();
	}).setHelp("Toggle quad view on/off");

	core::Command::registerCommand("toggleanimation", [this] (const core::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		_mainWindow->toggleAnimation();
	}).setHelp("Toggle animation view on/off");

	core::Command::registerCommand("resetcamera", [this] (const core::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		_mainWindow->resetCamera();
	}).setHelp("Reset cameras in all viewports");

	core::Command::registerCommand("dialog_noise", [this] (const core::CmdArgs& args) {
		if (_mainWindow == nullptr) {
			return;
		}
		tb::TBWidgetEvent event(tb::EVENT_TYPE_CUSTOM);
		event.ref_id = tb::TBGetHash("dialog_noise");
		_mainWindow->invokeEvent(event);
	}).setHelp("Opens the noise dialog");

	core::Var::get(cfg::RenderOutline, "false", core::CV_SHADER, "Render the scene with voxel outlines");

	return state;
}

core::AppState VoxEdit::onInit() {
	const core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!_sceneMgr.init()) {
		Log::error("Failed to initialize the scene manager");
		return core::AppState::InitFailure;
	}

	_mainWindow = new voxedit::VoxEditWindow(this);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return core::AppState::InitFailure;
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
			_sceneMgr.load(filePtr->name());
		}
	}

	return state;
}

core::AppState VoxEdit::onRunning() {
	core::AppState state = Super::onRunning();
	if (state != core::AppState::Running) {
		return state;
	}
	voxedit::sceneMgr().update(_now);
	_mainWindow->update();
	const bool isSceneHovered = _mainWindow->isSceneHovered();
	if (isSceneHovered) {
		core::setBindingContext(voxedit::BindingContext::Scene);
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
