/**
 * @file
 */

#include "VoxEdit.h"
#include "ui/MainWindow.h"
#include "core/Color.h"
#include "core/Command.h"
#include "video/GLFunc.h"

// TODO: voxelizer via assimp
// TODO: export via assimp exporters
// TODO: cursor volume shape generators
// TODO: palette widget
// TODO: voxel cursor tools
// TODO: it is possible to place a voxel outside the maxs region
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

	_mainWindow = new MainWindow(this);
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

	core::Command::registerCommand("export", [this] (const core::CmdArgs& args) {
		std::string_view file = args.empty() ? "" : args[0];
		if (!exportFile(file)) {
			Log::error("Failed to export to file %s", file.data());
		}
	}).setArgumentCompleter(fileCompleter).setHelp("Export the current state to the given file");

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
