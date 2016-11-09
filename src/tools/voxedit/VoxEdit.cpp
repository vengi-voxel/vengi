/**
 * @file
 */

#include "VoxEdit.h"
#include "ui/MainWindow.h"
#include "core/Color.h"
#include "core/Command.h"
#include "video/GLFunc.h"

// TODO: voxelizer via assimp
// TODO: cursor volume shape generators
// TODO: it is possible to place a voxel outside the maxs region
// TODO: extrude
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

	_mainWindow = new voxedit::MainWindow(this);
	if (!_mainWindow->init()) {
		Log::error("Failed to initialize the main window");
		return core::AppState::Cleanup;
	}

	auto fileCompleter = [=] (const std::string& str, std::vector<std::string>& matches) -> int {
		std::vector<io::Filesystem::DirEntry> entries;
		const std::string filter = str + "*";
		core::App::getInstance()->filesystem()->list(_lastDirectory->strVal(), entries, filter);
		int i = 0;
		for (const io::Filesystem::DirEntry& entry : entries) {
			if (entry.type == io::Filesystem::DirEntry::Type::file) {
				matches.push_back(entry.name);
				++i;
			}
		}
		return i;
	};

	core::Command::registerCommand("save", [this] (const core::CmdArgs& args) {
		std::string_view file = args.empty() ? "" : args[0];
		if (!saveFile(file)) {
			Log::error("Failed to save to file %s", file.data());
		}
	}).setArgumentCompleter(fileCompleter).setHelp("Save the current state to the given file");

	core::Command::registerCommand("toggleviewport", [this] (const core::CmdArgs& args) {
		this->_mainWindow->toggleQuadViewport();
	}).setArgumentCompleter(fileCompleter).setHelp("Toggle quad view on/off");

	core::Command::registerCommand("rotate", [this] (const core::CmdArgs& args) {
		const int size = args.size();
		const char axis = size < 1 ? 'x' : args[0][0];
		if (axis == 'x') {
			this->_mainWindow->rotateX();
		} else if (axis == 'y') {
			this->_mainWindow->rotateY();
		} else if (axis == 'z') {
			this->_mainWindow->rotateZ();
		} else {
			Log::info("Unknown axis given - valid options are x, y and z");
		}
	}).setArgumentCompleter(fileCompleter).setHelp("Rotate the volume");

	core::Command::registerCommand("export", [this] (const core::CmdArgs& args) {
		std::string_view file = args.empty() ? "" : args[0];
		if (!exportFile(file)) {
			Log::error("Failed to export to file %s", file.data());
		}
	}).setArgumentCompleter(fileCompleter).setHelp("Export the current state to the given file");

	core::Command::registerCommand("undo", [this] (const core::CmdArgs& args) {
		_mainWindow->undo();
	}).setArgumentCompleter(fileCompleter).setHelp("Undo your last step");

	core::Command::registerCommand("redo", [this] (const core::CmdArgs& args) {
		_mainWindow->redo();
	}).setArgumentCompleter(fileCompleter).setHelp("Redo your last step");

	core::Command::registerCommand("load", [this] (const core::CmdArgs& args) {
		std::string_view file = args.empty() ? "" : args[0];
		if (!loadFile(file)) {
			Log::error("Failed to load file %s", file.data());
		}
	}).setArgumentCompleter(fileCompleter).setHelp("Load a scene from the given file");

	core::Command::registerCommand("voxelize", [this] (const core::CmdArgs& args) {
		std::string_view file = args.empty() ? "" : args[0];
		if (!voxelizeFile(file)) {
			Log::error("Failed to voxelize file %s", file.data());
		}
	}).setArgumentCompleter(fileCompleter).setHelp("Load a scene from the given file");

	core::Command::registerCommand("new", [this] (const core::CmdArgs& args) {
		newFile(false);
	}).setHelp("Create a new scene");

	core::Command::registerCommand("select", [this] (const core::CmdArgs& args) {
		if (args.size() != 3) {
			return;
		}
		const int x = core::string::toInt(args[0]);
		const int y = core::string::toInt(args[1]);
		const int z = core::string::toInt(args[2]);
		const glm::ivec3 pos(x, y, z);
		select(pos);
	}).setHelp("Select voxels");

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

bool VoxEdit::onKeyPress(int32_t key, int16_t modifier) {
	if (Super::onKeyPress(key, modifier)) {
		return true;
	}
	if (key == SDLK_ESCAPE) {
		toggleRelativeMouseMode();
		return true;
	}
	if (_rotateMode) {
		if (key == SDLK_x) {
			_mainWindow->rotateX();
		} else if (key == SDLK_y) {
			_mainWindow->rotateY();
		} else if (key == SDLK_z) {
			_mainWindow->rotateZ();
		}
	}
	_rotateMode = key == SDLK_r;
	return false;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const video::MeshPoolPtr& meshPool = std::make_shared<video::MeshPool>();
	VoxEdit app(filesystem, eventBus, timeProvider, meshPool);
	return app.startMainLoop(argc, argv);
}
