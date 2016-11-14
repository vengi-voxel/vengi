/**
 * @file
 */

#include "VoxEdit.h"
#include "core/Color.h"
#include "core/Command.h"
#include "video/GLFunc.h"
#include "ui/VoxEditWindow.h"

#define COMMAND_MAINWINDOW(command, help) core::Command::registerCommand(#command, [this] (const core::CmdArgs& args) {_mainWindow->command();}).setHelp(help)
#define COMMAND_FILE(command, help) \
	core::Command::registerCommand(#command, [this] (const core::CmdArgs& args) { \
		std::string_view file = args.empty() ? "" : args[0]; \
		if (!command##File(file)) { \
			Log::error("Failed to " #command " to file %s", file.data()); \
		} \
	}).setArgumentCompleter(fileCompleter).setHelp(help)
#define COMMAND_CALL(command, call, help) core::Command::registerCommand(command, [this] (const core::CmdArgs& args) {call;}).setHelp(help)

// TODO: voxelizer via assimp
// TODO: extrude
// TODO: fill tool
// TODO: render locked axis as plane
// TODO: shift locked axis
// TODO: scale/move/rotate selections - not only the cursor
VoxEdit::VoxEdit(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const video::MeshPoolPtr& meshPool) :
		ui::UIApp(filesystem, eventBus, timeProvider), _mainWindow(nullptr), _meshPool(meshPool) {
	init("engine", "voxedit");
}

bool VoxEdit::saveFile(std::string_view file) {
	return _mainWindow->save(file);
}

bool VoxEdit::loadFile(std::string_view file) {
	return _mainWindow->load(file);
}

bool VoxEdit::voxelizeFile(std::string_view file) {
	return _mainWindow->voxelize(file);
}

void VoxEdit::select(const glm::ivec3& pos) {
	_mainWindow->select(pos);
}

bool VoxEdit::newFile(bool force) {
	return _mainWindow->createNew(force);
}

bool VoxEdit::exportFile(std::string_view file) {
	return _mainWindow->exportFile(file);
}

core::AppState VoxEdit::onCleanup() {
	const core::AppState state = Super::onCleanup();
	_meshPool->shutdown();
	return state;
}

core::AppState VoxEdit::onInit() {
	const core::AppState state = Super::onInit();

	_lastDirectory = core::Var::get("ve_lastdirectory", core::App::getInstance()->filesystem()->homePath().c_str());

	_meshPool->init();

	_mainWindow = new voxedit::VoxEditWindow(this);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return core::AppState::Cleanup;
	}

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

	core::Command::registerCommand("select", [this] (const core::CmdArgs& args) {
		if (args.size() != 3) {
			Log::info("Expected to get x, y and z coordinates");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		const glm::ivec3 pos(x, y, z);
		select(pos);
	}).setHelp("Select voxels from the given position");

	core::Command::registerCommand("togglerelativemousemode", [this] (const core::CmdArgs& args) {
		toggleRelativeMouseMode();
	}).setHelp("Toggle relative mouse mode which provides free look");

	core::Command::registerCommand("rotate", [this] (const core::CmdArgs& args) {
		if (args.size() != 3) {
			Log::info("Expected to get x, y and z angles in degrees");
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		this->_mainWindow->rotate(x, y, z);
	}).setHelp("Select voxels from the given position");

	COMMAND_CALL("new", newFile(), "Create a new scene");

	COMMAND_FILE(save, "Save the current state to the given file");
	COMMAND_FILE(export, "Export the current state to the given file");
	COMMAND_FILE(load, "Load a scene from the given file");
	COMMAND_FILE(voxelize, "Load a scene from the given file");

	COMMAND_MAINWINDOW(unselectall, "Unselect every voxel");
	COMMAND_MAINWINDOW(rotatex, "Rotate the volume around the x axis");
	COMMAND_MAINWINDOW(rotatey, "Rotate the volume around the y axis");
	COMMAND_MAINWINDOW(rotatez, "Rotate the volume around the z axis");
	COMMAND_MAINWINDOW(scalex, "Scale the cursor volume in x direction");
	COMMAND_MAINWINDOW(scaley, "Scale the cursor volume in y direction");
	COMMAND_MAINWINDOW(scalez, "Scale the cursor volume in z direction");
	COMMAND_MAINWINDOW(crop, "Crop your volume");
	COMMAND_MAINWINDOW(extend, "Extend your volume");
	COMMAND_MAINWINDOW(undo, "Undo your last step");
	COMMAND_MAINWINDOW(redo, "Redo your last step");
	COMMAND_MAINWINDOW(copy, "Copy selection into cursor");
	COMMAND_MAINWINDOW(paste, "Insert cursor volume into model volume");
	COMMAND_MAINWINDOW(cut, "Delete selected volume from model volume");
	COMMAND_MAINWINDOW(toggleviewport, "Toggle quad view on/off");
	COMMAND_MAINWINDOW(togglefreelook, "Toggle free look on/off");
	COMMAND_MAINWINDOW(rotatemode, "Activates the rotate mode");
	COMMAND_MAINWINDOW(scalemode, "Activates the scale mode");
	COMMAND_MAINWINDOW(movemode, "Activates the move mode");

	newFile(true);

	const glm::vec4& color = ::core::Color::Black;
	glClearColor(color.r, color.g, color.b, color.a);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	setRelativeMouseMode(false);

	return state;
}

core::AppState VoxEdit::onRunning() {
	core::AppState state = Super::onRunning();
	if (state == core::AppState::Cleanup) {
		return state;
	}
	const bool current = isRelativeMouseMode();
	if (current) {
		centerMouseCursor();
	}
	return state;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const video::MeshPoolPtr& meshPool = std::make_shared<video::MeshPool>();
	VoxEdit app(filesystem, eventBus, timeProvider, meshPool);
	return app.startMainLoop(argc, argv);
}
