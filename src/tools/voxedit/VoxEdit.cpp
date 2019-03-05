/**
 * @file
 */

#include "VoxEdit.h"
#include "core/Color.h"
#include "voxel/MaterialColor.h"
#include "core/command/Command.h"
#include "video/Renderer.h"
#include "ui/VoxEditWindow.h"
#include "io/Filesystem.h"

#define COMMAND_MAINWINDOW(command, help) core::Command::registerCommand(#command, [this] (const core::CmdArgs& args) {_mainWindow->command();}).setHelp(help)
#define COMMAND_FILE(command, help) \
	core::Command::registerCommand(#command, [this] (const core::CmdArgs& args) { \
		const std::string file = args.empty() ? "" : args[0]; \
		if (!command##File(file)) { \
			Log::error("Failed to execute '" #command "' for file %s", file.c_str()); \
		} \
	}).setArgumentCompleter(fileCompleter).setHelp(help)
#define COMMAND_CALL(command, call, help) core::Command::registerCommand(command, [this] (const core::CmdArgs& args) {call;}).setHelp(help)
#define COMMAND_MAINWINDOW_EVENT(command, help) core::Command::registerCommand(command, [this] (const core::CmdArgs& args) {tb::TBWidgetEvent event(tb::EVENT_TYPE_CUSTOM);event.ref_id = TBIDC(command);_mainWindow->invokeEvent(event);}).setHelp(help)

VoxEdit::VoxEdit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const video::MeshPoolPtr& meshPool) :
		Super(metric, filesystem, eventBus, timeProvider), _mainWindow(nullptr), _meshPool(meshPool) {
	init(ORGANISATION, "voxedit");
	_allowRelativeMouseMode = false;
}

bool VoxEdit::importheightmapFile(const std::string& file) {
	return _mainWindow->importHeightmap(file);
}

bool VoxEdit::saveFile(const std::string& file) {
	return _mainWindow->save(file);
}

bool VoxEdit::screenshotFile(const std::string& file) {
	return _mainWindow->saveScreenshot(file);
}

bool VoxEdit::loadFile(const std::string& file) {
	return _mainWindow->load(file);
}

bool VoxEdit::prefabFile(const std::string& file) {
	return _mainWindow->prefab(file);
}

bool VoxEdit::importmeshFile(const std::string& file) {
	return _mainWindow->importMesh(file);
}

bool VoxEdit::newFile(bool force) {
	return _mainWindow->createNew(force);
}

bool VoxEdit::exportFile(const std::string& file) {
	return _mainWindow->exportFile(file);
}

core::AppState VoxEdit::onCleanup() {
	const core::AppState state = Super::onCleanup();
	_meshPool->shutdown();
	_sceneMgr.shutdown();
	return state;
}

core::AppState VoxEdit::onConstruct() {
	const core::AppState state = Super::onConstruct();

	_sceneMgr.construct();

	auto fileCompleter = [=] (const std::string& str, std::vector<std::string>& matches) -> int {
		const std::string& dir = _lastDirectory->strVal().empty() ? "." : _lastDirectory->strVal();
		std::vector<io::Filesystem::DirEntry> entries;
		const std::string filter = str + "*";
		core::App::getInstance()->filesystem()->list(dir, entries, filter);
		int i = 0;
		for (const io::Filesystem::DirEntry& entry : entries) {
			if (entry.type == io::Filesystem::DirEntry::Type::file) {
				matches.push_back(entry.name);
				++i;
			}
		}
		return i;
	};

	COMMAND_MAINWINDOW_EVENT("dialog_noise", "Opens the noise dialog");

	COMMAND_CALL("new", newFile(), "Create a new scene");

	COMMAND_FILE(screenshot, "Save the current viewport as screenshot");
	COMMAND_FILE(save, "Save the current state to the given file");
	COMMAND_FILE(export, "Export the current state to the given file");
	COMMAND_FILE(load, "Load a scene from the given file");
	COMMAND_FILE(prefab, "Add a model to the existing scene from the given file");
	COMMAND_FILE(importmesh, "Import a mesh from the given file and tries to voxelize it");
	COMMAND_FILE(importheightmap, "Import a heightmap into the volume");

	COMMAND_MAINWINDOW(toggleviewport, "Toggle quad view on/off");
	COMMAND_MAINWINDOW(resetcamera, "Reset cameras");

	return state;
}

core::AppState VoxEdit::onInit() {
	const core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}

	if (!_meshPool->init()) {
		Log::error("Failed to initialize the mesh pool");
		return core::AppState::InitFailure;
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

	return state;
}

core::AppState VoxEdit::onRunning() {
	core::AppState state = Super::onRunning();
	if (state != core::AppState::Running) {
		return state;
	}
	_sceneMgr.update(_now);
	_mainWindow->update();
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const video::MeshPoolPtr& meshPool = std::make_shared<video::MeshPool>();
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	VoxEdit app(metric, filesystem, eventBus, timeProvider, meshPool);
	return app.startMainLoop(argc, argv);
}
