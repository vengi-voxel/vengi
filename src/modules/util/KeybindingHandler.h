/**
 * @file
 */

#pragma once

#include "KeybindingParser.h"
#include <unordered_set>
#include <stdint.h>

namespace util {

/**
 * @param bindings Map of bindings
 * @param key The key that was pressed
 * @param modifier The modifier mask
 * @return @c true if the key+modifier combination lead to a command execution via
 * key bindings, @c false otherwise
 */
extern bool executeCommandsForBinding(const BindMap& bindings, int32_t key, int16_t modifier, uint64_t now = 0ul);

extern bool isValidForBinding(int16_t pressedModMask, int16_t commandModMask);

class KeyBindingHandler : public core::IComponent {
private:
	uint32_t _pressedModifierMask = 0u;
	std::unordered_set<int32_t> _keys;
	BindMap _bindings;

	/**
	 * @brief Reverse lookup of key bindings - by command name
	 * @param[out] modifier The modifier mask that the command is bound to
	 * @param[out] key The key that the command is bound to
	 * @return @c false if no binding for the given command was found
	 * @note Only use the values of the given input pointers, if the
	 * method returned @c true
	 */
	bool resolveKeyBindings(const char *cmd, int16_t* modifier, int32_t* key) const;

	inline bool isPressed(int32_t key) const {
		return _keys.find(key) != _keys.end();
	}

	/**
	 * @brief Tries to identify commands with several modifier masks.
	 */
	bool executeCommands(int32_t key, int16_t modifier, uint64_t now);

public:
	void construct() override;
	void shutdown() override;
	bool init() override;

	static const char* getModifierName(int16_t modifier);
	static const char* getKeyName(uint32_t key);
	bool load(const std::string& name);

	bool execute(int32_t key, int16_t modifier, bool pressed, uint64_t now);
	std::string getKeyBindingsString(const char *cmd) const;
};

}
