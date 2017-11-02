#include "CommandHandler.h"
#include "core/command/Command.h"
#include "core/Tokenizer.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/String.h"

namespace core {

int executeCommands(const std::string& _commandLine) {
	if (_commandLine.empty()) {
		return 0;
	}
	int n = 0;
	const core::Tokenizer tok(_commandLine, ";");
	for (const std::string& command : tok.tokens()) {
		if (command.empty()) {
			continue;
		}
		const std::string_view trimmed = core::string::trim(command);
		if (trimmed.empty()) {
			continue;
		}
		const core::Tokenizer tokInner(trimmed, " ");
		if (tokInner.tokens().empty()) {
			continue;
		}
		std::vector<std::string> tokens = tokInner.tokens();
		const std::string cmd = tokens[0];
		tokens.erase(tokens.begin());
		if (core::Command::execute(cmd, tokens)) {
			if (n != -1) {
				++n;
			}
			continue;
		}
		const core::VarPtr& c = core::Var::get(cmd);
		if (!c) {
			Log::info("unknown command: %s", cmd.c_str());
			n = -1;
		} else {
			if (tokens.empty()) {
				if (c->strVal().empty()) {
					Log::info("%s: no value set", cmd.c_str());
				} else {
					Log::info("%s: %s", cmd.c_str(), c->strVal().c_str());
				}
			} else {
				c->setVal(core::string::eraseAllSpaces(tokens[0]));
			}
		}
		if (n != -1) {
			++n;
		}
	}
	return n;
}

}
