/**
 * @file
 */

#include "CommandHandler.h"
#include "core/command/Command.h"
#include "core/Tokenizer.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/String.h"
#include <algorithm>
#include <vector>

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

// https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#C++
size_t levensteinDistance(const std::string &source, const std::string &target) {
	if (source.size() > target.size()) {
		return levensteinDistance(target, source);
	}

	const size_t minSize = source.size(), maxSize = target.size();
	std::vector<size_t> levDist(minSize + 1);

	for (size_t i = 0; i <= minSize; ++i) {
		levDist[i] = i;
	}

	for (size_t j = 1; j <= maxSize; ++j) {
		size_t previousDiagonal = levDist[0], previousDiagonalSave;
		++levDist[0];

		for (size_t i = 1; i <= minSize; ++i) {
			previousDiagonalSave = levDist[i];
			if (source[i - 1] == target[j - 1]) {
				levDist[i] = previousDiagonal;
			} else {
				levDist[i] = std::min(std::min(levDist[i - 1], levDist[i]),
						previousDiagonal) + 1;
			}
			previousDiagonal = previousDiagonalSave;
		}
	}

	return levDist[minSize];
}

static const char* findPotentialMatch(const std::string& arg) {
	const char *match = nullptr;
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
			const char *potentialMatch = findPotentialMatch(cmd);
			if (potentialMatch != nullptr) {
				Log::info("did you mean: %s", potentialMatch);
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
