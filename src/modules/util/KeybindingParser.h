/**
 * @file
 */

#pragma once

#include "core/Tokenizer.h"
#include <SDL.h>
#include <unordered_map>

namespace util {

struct CommandModifierPair {
	inline CommandModifierPair(const std::string& _command, int16_t _modifier) :
			command(_command), modifier(_modifier) {
	}
	std::string command;
	int16_t modifier;
};
typedef std::unordered_multimap<int32_t, CommandModifierPair> BindMap;

class KeybindingParser: public core::Tokenizer {
private:
	BindMap _bindings;
	int _invalidBindings;

	void parseKeyAndCommand(std::string key, const std::string& command);
public:
	KeybindingParser(const std::string& key, const std::string& binding);

	KeybindingParser(const std::string& bindings);

	inline int invalidBindings() const {
		return _invalidBindings;
	}

	inline const BindMap& getBindings() const {
		return _bindings;
	}
};

}
