/**
 * @file
 */

#include "CommandHandler.h"
#include "core/BindingContext.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "command/Command.h"
#include "core/Tokenizer.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/StringUtil.h"

namespace command {

static core::String findPotentialMatch(const core::String& arg) {
	core::String match;
	size_t leastCost = 1000000u;
	command::Command::visit([&] (const command::Command& c) {
		const size_t cost = core::string::levenshteinDistance(arg, c.name());
		if (cost < leastCost) {
			leastCost = cost;
			match = c.name();
		}
	});

	return match;
}

int executeCommands(const core::String& commandLine, CommandExecutionListener *listener) {
	if (commandLine.empty()) {
		return 0;
	}
	core_trace_scoped(ExecuteCommands);
	int n = 0;
	core::TokenizerConfig cfg;
	cfg.skipComments = false;
	cfg.removeQuotes = false;
	const core::Tokenizer tok(cfg, commandLine, ";");
	for (const core::String& command : tok.tokens()) {
		if (command.empty()) {
			continue;
		}
		const core::String& trimmed = core::string::trim(command);
		if (trimmed.empty()) {
			continue;
		}
		core::TokenizerConfig innerCfg;
		innerCfg.skipComments = false;
		const core::Tokenizer tokInner(innerCfg, trimmed, " ");
		if (tokInner.tokens().empty()) {
			continue;
		}
		core::Tokens tokens = tokInner.tokens();
		const core::String cmd = tokens[0];
		tokens.erase(tokens.begin());
		if (listener && !listener->allowed(cmd, tokens)) {
			continue;
		}
		if (command::Command::execute(cmd, tokens)) {
			if (n != -1) {
				++n;
				if (listener) {
					(*listener)(cmd, tokens);
				}
			}
			continue;
		}
		const core::VarPtr& c = core::Var::get(cmd);
		if (!c) {
			Log::info("unknown command: %s in binding context %i", cmd.c_str(), (int)core::bindingContext());
			const core::String& potentialMatch = findPotentialMatch(cmd);
			if (!potentialMatch.empty()) {
				Log::info("did you mean: %s", potentialMatch.c_str());
			}
			n = -1;
		} else {
			if (tokens.empty()) {
				if (c->strVal().empty()) {
					Log::info("%s: no value set", cmd.c_str());
				} else {
					Log::info("%s: %s", cmd.c_str(), c->strVal().c_str());
				}
				if (c->help() != nullptr) {
					Log::info("- %s", c->help());
				}
			} else {
				const core::String& value = core::string::join(tokens.begin(), tokens.end(), " ");
				Log::debug("%s = %s", c->name().c_str(), value.c_str());
				c->setVal(value);
			}
		}
		if (n != -1) {
			++n;
		}
	}
	return n;
}

}
