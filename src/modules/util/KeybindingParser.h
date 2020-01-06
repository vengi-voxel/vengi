/**
 * @file
 */

#pragma once

#include "core/Tokenizer.h"
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

/**
 * @brief Parses keys/command combinations
 */
class KeybindingParser: public core::Tokenizer {
private:
	BindMap _bindings;
	int _invalidBindings;

	void parseKeyAndCommand(std::string key, const std::string& command);
public:
	/**
	 * @brief Parses a single binding
	 */
	KeybindingParser(const std::string& key, const std::string& binding);

	/**
	 * @brief Parses a buffer of bindings. Each binding is separated by a newline.
	 */
	KeybindingParser(const std::string& bindings);

	/**
	 * @return The amount of invalid bindings.
	 * @note Invalid bindings are defined by invalid key names.
	 */
	inline int invalidBindings() const {
		return _invalidBindings;
	}

	/**
	 * @return The map of parsed bindings
	 */
	inline const BindMap& getBindings() const {
		return _bindings;
	}
};

}
