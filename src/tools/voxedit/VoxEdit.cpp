/**
 * @file
 */

#include "VoxEdit.h"
#include "core/Color.h"
#include "voxel/MaterialColor.h"
#include "core/command/Command.h"
#include "ui/editorscene/ViewportSingleton.h"
#include "video/Renderer.h"
#include "ui/VoxEditWindow.h"
#include "io/Filesystem.h"

#define COMMAND_VIEWPORTSINGLETON(command, help) core::Command::registerCommand(#command, [this] (const core::CmdArgs& args) {::voxedit::ViewportSingleton::getInstance().command();}).setHelp(help)
#define COMMAND_MAINWINDOW(command, help) core::Command::registerCommand(#command, [this] (const core::CmdArgs& args) {_mainWindow->command();}).setHelp(help)
#define COMMAND_FILE(command, help) \
	core::Command::registerCommand(#command, [this] (const core::CmdArgs& args) { \
		const std::string file = args.empty() ? "" : args[0]; \
		if (!command##File(file)) { \
			Log::error("Failed to execute '" #command "' for file %s", file.c_str()); \
		} \
	}).setArgumentCompleter(fileCompleter).setHelp(help)
#define COMMAND_CALL(command, call, help) core::Command::registerCommand(command, [this] (const core::CmdArgs& args) {call;}).setHelp(help)
#define COMMAND_MAINWINDOW_EVENT(command, help) core::Command::registerCommand(command, [this] (const core::CmdArgs& args) {tb::TBWidgetEvent event(tb::EVENT_TYPE_CUSTOM);event.ref_id = TBIDC(command);_mainWindow->InvokeEvent(event);}).setHelp(help)

VoxEdit::VoxEdit(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const video::MeshPoolPtr& meshPool) :
		Super(metric, filesystem, eventBus, timeProvider), _mainWindow(nullptr), _meshPool(meshPool) {
	init(ORGANISATION, "voxedit");
}

bool VoxEdit::importheightmapFile(const std::string& file) {
	return _mainWindow->importHeightmap(file);
}

bool VoxEdit::saveFile(const std::string& file) {
	return _mainWindow->save(file);
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
	return state;
}

core::AppState VoxEdit::onConstruct() {
	const core::AppState state = Super::onConstruct();
	_lastDirectory = core::Var::get("ve_lastdirectory", core::App::getInstance()->filesystem()->homePath().c_str());

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

	core::Command::registerCommand("rotate", [this] (const core::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get x, y and z angles in degrees");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		this->_mainWindow->rotate(x, y, z);
	}).setHelp("Rotate voxels by the given angles (in degree)");

	core::Command::registerCommand("cursor", [this] (const core::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		this->_mainWindow->setCursorPosition(x, y, z, false);
	}).setHelp("Set the cursor to the specified position");

	for (size_t i = 0; i < lengthof(DIRECTIONS); ++i) {
		core::Command::registerActionButton(core::string::format("movecursor%s", DIRECTIONS[i].postfix), _move[i]);
		_lastMove[i] = _now;
	}

	core::Command::registerCommand("movecursor", [this] (const core::CmdArgs& args) {
		if (args.size() < 3) {
			Log::info("Expected to get relative x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		this->_mainWindow->setCursorPosition(x, y, z, true);
	}).setHelp("Move the cursor by the specified offsets");

	core::Command::registerCommand("setreferenceposition", [this] (const core::CmdArgs& args) {
		if (args.size() != 3) {
			Log::info("Expected to get x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		this->_mainWindow->setReferencePosition(x, y, z);
	}).setHelp("Set the reference position to the specified position");

	COMMAND_MAINWINDOW_EVENT("dialog_noise", "Opens the noise dialog");

	COMMAND_CALL("new", newFile(), "Create a new scene");

	COMMAND_FILE(save, "Save the current state to the given file");
	COMMAND_FILE(export, "Export the current state to the given file");
	COMMAND_FILE(load, "Load a scene from the given file");
	COMMAND_FILE(prefab, "Add a model to the existing scene from the given file");
	COMMAND_FILE(importmesh, "Import a mesh from the given file and tries to voxelize it");
	COMMAND_FILE(importheightmap, "Import a heightmap into the volume");

	COMMAND_MAINWINDOW(setreferencepositiontocursor, "Set the reference position to the current cursor position");
	COMMAND_MAINWINDOW(rotatex, "Rotate the volume around the x axis");
	COMMAND_MAINWINDOW(rotatey, "Rotate the volume around the y axis");
	COMMAND_MAINWINDOW(rotatez, "Rotate the volume around the z axis");
	COMMAND_VIEWPORTSINGLETON(crop, "Crop your volume");
	core::Command::registerCommand("resize", [this] (const core::CmdArgs& args) {
		const int argc = args.size();
		if (argc == 1) {
			const int size = core::string::toInt(args[0]);
			_mainWindow->extend(glm::ivec3(size));
		} else if (argc >= 2) {
			glm::ivec3 size;
			for (int i = 0; i < argc; ++i) {
				size[i] = core::string::toInt(args[i]);
			}
			_mainWindow->extend(size);
		} else {
			_mainWindow->extend();
		}
	}).setHelp("Resize your volume about given x, y and z size");
	COMMAND_VIEWPORTSINGLETON(scaleHalf, "Scale your volume by 50%");
	COMMAND_MAINWINDOW(undo, "Undo your last step");
	COMMAND_MAINWINDOW(redo, "Redo your last step");
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

	_meshPool->init();

	_mainWindow = new voxedit::VoxEditWindow(this);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return core::AppState::InitFailure;
	}

	newFile(true);

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

void VoxEdit::update() {
	for (size_t i = 0; i < lengthof(DIRECTIONS); ++i) {
		if (!_move[i].pressed()) {
			continue;
		}
		if (_now - _lastMove[i] < 125ul) {
			continue;
		}
		const Direction& dir = DIRECTIONS[i];
		this->_mainWindow->setCursorPosition(dir.x, dir.y, dir.z, true);
		_lastMove[i] = _now;
	}
	_mainWindow->update();
}

core::AppState VoxEdit::onRunning() {
	core::AppState state = Super::onRunning();
	if (state != core::AppState::Running) {
		return state;
	}
	const bool current = isRelativeMouseMode();
	if (current) {
		centerMousePosition();
	}
	update();
	return state;
}

bool VoxEdit::onKeyPress(int32_t key, int16_t modifier) {
	if (Super::onKeyPress(key, modifier)) {
		return true;
	}
	if (key == SDLK_ESCAPE) {
		toggleRelativeMouseMode();
		return true;
	}
	return false;
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
