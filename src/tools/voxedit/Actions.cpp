#include "Actions.h"
#include "VoxEdit.h"
#include "core/Command.h"

void registerActions(VoxEdit* tool, const core::VarPtr& lastDirectory) {
	auto fileCompleter = [=] (const std::string& str, std::vector<std::string>& matches) -> int {
		std::vector<io::Filesystem::DirEntry> entries;
		const std::string filter = str + "*";
		core::App::getInstance()->filesystem()->list(lastDirectory->strVal(), entries, filter);
		int i = 0;
		for (const io::Filesystem::DirEntry& entry : entries) {
			if (entry.type == io::Filesystem::DirEntry::Type::file) {
				matches.push_back(entry.name);
				++i;
			}
		}
		return i;
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
