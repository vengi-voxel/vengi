/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "KeybindingParser.h"
#include <unordered_set>
#include <stdint.h>

namespace util {

extern bool isValidForBinding(int16_t pressedModMask, int16_t commandModMask);

/**
 * @brief Component that executes commands for key/modifier combinations.
 */
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

	/**
	 * @brief Tries to identify commands with several modifier masks.
	 */
	bool executeCommands(int32_t key, int16_t modifier, double nowSeconds);

	static core::String getKeyName(int32_t key);
	static const char* getModifierName(int16_t modifier);
public:
	void construct() override;
	void shutdown() override;
	bool init() override;

	/**
	 * @brief Print the binding line for a key/modifier combination
	 */
	static core::String toString(int32_t key, int16_t modifier);
	/**
	 * @brief Resolve the bindings for a given command string
	 * @return Empty string if the given command doesn't have a binding, or the value
	 * of @c toString(key, modifier) if a binding was found.
	 */
	core::String getKeyBindingsString(const char *cmd) const;

	/**
	 * @brief Loads a keybindings file
	 */
	bool load(const core::String& name);
	void setBindings(const BindMap& bindings);

	bool isPressed(int32_t key) const;

	/**
	 * @brief Executes a key up/down commands for a given key/modifier combination
	 */
	bool execute(int32_t key, int16_t modifier, bool pressed, double nowSeconds);
};

inline bool KeyBindingHandler::isPressed(int32_t key) const {
	return _keys.find(key) != _keys.end();
}

}
