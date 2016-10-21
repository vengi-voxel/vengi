#include "Actions.h"
#include "VoxEdit.h"
#include "core/Command.h"

void registerActions(VoxEdit* tool) {
	auto fileCompleter = [] (const std::string& str, std::vector<std::string>& matches) -> int {
		// TODO:
		// core::App::getInstance()->filesystem()->listDir(_lastDirectory->strVal());
		return 0;
	};

	core::Command::registerCommand("save", [tool] (const core::CmdArgs& args) {
		if (args.empty()) {
			Log::error("Usage: save <filename>");
			return;
		}
		if (!tool->saveFile(args[0])) {
			Log::error("Failed to save to file %s", args[0].c_str());
		}
	}).setArgumentCompleter(fileCompleter).setHelp("Save the current state to the given file");

	core::Command::registerCommand("load", [tool] (const core::CmdArgs& args) {
		if (args.empty()) {
			Log::error("Usage: load <filename>");
			return;
		}
		if (!tool->loadFile(args[0])) {
			Log::error("Failed to load file %s", args[0].c_str());
		}
	}).setArgumentCompleter(fileCompleter).setHelp("Load a scene from the given file");

	core::Command::registerCommand("new", [tool] (const core::CmdArgs& args) {
		tool->newFile(false);
	}).setHelp("Create a new scene");
}
