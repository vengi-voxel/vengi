/**
 * @file
 */

#include "CommandHandler.h"
#include "core/command/Command.h"
#include "core/Tokenizer.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/StringUtil.h"
#include <vector>

namespace core {

bool replacePlaceholders(const core::String& str, char *buf, size_t bufSize) {
	int idx = 0;
	for (size_t i = 0u; i < str.size(); ++i) {
		const char *c = &str.c_str()[i];
		if (SDL_strncmp(c, "<cvar:", 6) == 0) {
			const char *l = SDL_strchr(c, '>');
			if (l != nullptr) {
				c += 6;
				const intptr_t len = l - c;
				const core::String name(c, len);
				core_assert((int)len == (int)name.size());
				const core::VarPtr& var = core::Var::get(name);
				i += 6 + len;
				const core::String& value = var->strVal();
				const size_t remaining = bufSize - idx - 1;
				SDL_strlcpy(&buf[idx], value.c_str(), remaining);
				idx += (int)value.size();
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

// https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C++
size_t levensteinDistance(const core::String &source, const core::String &target) {
	if (source.size() > target.size()) {
		return levensteinDistance(target, source);
	}

	const size_t minSize = source.size();
	const size_t maxSize = target.size();
	std::vector<size_t> levDist(minSize + 1);

	for (size_t i = 0; i <= minSize; ++i) {
		levDist[i] = i;
	}

	for (size_t j = 1; j <= maxSize; ++j) {
		size_t previousDiagonal = levDist[0];
		++levDist[0];

		for (size_t i = 1; i <= minSize; ++i) {
			const size_t previousDiagonalSave = levDist[i];
			if (source[i - 1] == target[j - 1]) {
				levDist[i] = previousDiagonal;
			} else {
				levDist[i] = core_min(core_min(levDist[i - 1], levDist[i]),
						previousDiagonal) + 1;
			}
			previousDiagonal = previousDiagonalSave;
		}
	}

	return levDist[minSize];
}

static core::String findPotentialMatch(const core::String& arg) {
	core::String match;
	size_t leastCost = 1000000u;
	core::Command::visit([&] (const core::Command& c) {
		const size_t cost = levensteinDistance(arg, c.name());
		if (cost < leastCost) {
			leastCost = cost;
			match = c.name();
		}
	});

	return match;
}

int executeCommands(const core::String& commandLine) {
	if (commandLine.empty()) {
		return 0;
	}
	int n = 0;
	const core::Tokenizer tok(false, commandLine, ";");
	for (const core::String& command : tok.tokens()) {
		if (command.empty()) {
			continue;
		}
		const core::String& trimmed = core::string::trim(command);
		if (trimmed.empty()) {
			continue;
		}
		char buf[512];
		replacePlaceholders(trimmed, buf, sizeof(buf));
		const core::Tokenizer tokInner(false, buf, " ");
		if (tokInner.tokens().empty()) {
			continue;
		}
		std::vector<core::String> tokens = tokInner.tokens();
		const core::String cmd = tokens[0];
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
