/**
 * @file
 */

#include "VoxEdit.h"
#include "ui/MainWindow.h"
#include "core/Color.h"
#include "core/Command.h"
#include "video/GLFunc.h"

VoxEdit::VoxEdit(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		ui::UIApp(filesystem, eventBus, timeProvider), _mainWindow(nullptr) {
	init("engine", "voxedit");
}

bool VoxEdit::saveFile(std::string_view file) {
	return _mainWindow->save(file);
}

bool VoxEdit::loadFile(std::string_view file) {
	return _mainWindow->load(file);
}

bool VoxEdit::newFile(bool force) {
	return _mainWindow->createNew(force);
}

core::AppState VoxEdit::onInit() {
	const core::AppState state = Super::onInit();

	_lastDirectory = core::Var::get("ve_lastdirectory", core::App::getInstance()->filesystem()->homePath().c_str());

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
		if (args.empty()) {
			Log::error("Usage: save <filename>");
			return;
		}
		if (!saveFile(args[0])) {
			Log::error("Failed to save to file %s", args[0].c_str());
		}
	}).setArgumentCompleter(fileCompleter).setHelp("Save the current state to the given file");

	core::Command::registerCommand("load", [this] (const core::CmdArgs& args) {
		if (args.empty()) {
			Log::error("Usage: load <filename>");
			return;
		}
		if (!loadFile(args[0])) {
			Log::error("Failed to load file %s", args[0].c_str());
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
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	VoxEdit app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
