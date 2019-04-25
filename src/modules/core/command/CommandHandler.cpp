/**
 * @file
 */

#include "CommandHandler.h"
#include "core/command/Command.h"
#include "core/Tokenizer.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/String.h"

namespace core {

bool replacePlaceholders(std::string_view str, char *buf, size_t bufSize) {
	int idx = 0;
	for (size_t i = 0u; i < str.length(); ++i) {
		const char *c = &str[i];
		if (!strncmp(c, "<cvar:", 6)) {
			const char *l = strchr(c, '>');
			if (l != nullptr) {
				c += 6;
				const intptr_t len = l - c;
				const std::string name(c, len);
				core_assert((int)len == (int)name.size());
				const core::VarPtr& var = core::Var::get(name);
				i += 6 + len;
				const std::string& value = var->strVal();
				const size_t remaining = bufSize - idx - 1;
				strncpy(&buf[idx], value.c_str(), remaining);
				idx += value.length();
				if (idx >= (int)bufSize) {
					return false;
				}
				continue;
			}
		}
		buf[idx++] = *c;
		if (idx >= (int)bufSize) {
			return false;
		}
	}
	if (idx >= (int)bufSize) {
		return false;
	}
	buf[idx] = '\0';
	return true;
}

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
		char buf[512];
		replacePlaceholders(trimmed, buf, sizeof(buf));
		const core::Tokenizer tokInner(buf, " ");
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
				c->setVal(core::string::join(tokens.begin(), tokens.end(), " "));
			}
		}
		if (n != -1) {
			++n;
		}
	}
	return n;
}

}
