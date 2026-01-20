/**
 * @file
 */

#pragma once

#include "core/BindingContext.h"
#include "core/IComponent.h"
#include "KeybindingParser.h"
#include "core/collection/Set.h"
#include <stdint.h>

namespace util {

bool isValidForBinding(int16_t pressedModMask, int16_t commandModMask);

/**
 * @brief Component that executes commands for key/modifier combinations.
 */
class KeyBindingHandler {
private:
	uint32_t _pressedModifierMask = 0u;
	core::Set<int32_t> _keys;
	BindMap _bindings;

	/**
	 * @brief Reverse lookup of key bindings - by command name
	 * @param[out] modifier The modifier mask that the command is bound to
	 * @param[out] key The key that the command is bound to
	 * @return @c false if no binding for the given command was found
	 * @note Only use the values of the given input pointers, if the
	 * method returned @c true
	 */
	bool resolveKeyBindings(const char *cmd, int16_t* modifier, int32_t* key, uint16_t *count) const;

	/**
	 * @brief Tries to identify commands with several modifier masks.
	 */
	bool executeCommands(int32_t key, int16_t modifier, double nowSeconds, uint16_t count);
	void saveKeybindings(int version);
	static core::String getKeyName(int32_t key, uint16_t count = 1u);
	static const char* getModifierName(int16_t modifier);
public:
	void construct();
	void shutdown(int version);
	bool init();
	void clear();
	void reset(int version);
	void openKeybindings(int version);
	void removeApplicationKeyBindings(int version);

	bool registerBinding(const core::String &command, int32_t key, int16_t modifier,
						 core::BindingContext context, uint16_t count);

	bool registerBinding(const core::String &keys, const core::String &command,
						 core::BindingContext context);

	bool registerBinding(const core::String &keys, const core::String &command,
						 const core::String &context);


	/**
	 * @brief Print the binding line for a key/modifier combination
	 */
	static core::String toString(int32_t key, int16_t modifier, uint16_t count);
	/**
	 * @brief Resolve the bindings for a given command string
	 * @return Empty string if the given command doesn't have a binding, or the value
	 * of @c toString(key, modifier) if a binding was found.
	 */
	core::String getKeyBindingsString(const char *cmd) const;

	/**
	 * @brief Loads a keybindings file
	 */
	bool load(int version);
	/**
	 * @brief Loads a keybindings string
	 */
	bool loadBindings(const core::String &bindings);
	void setBindings(const BindMap& bindings);
	const BindMap bindings() const;

	bool isPressed(int32_t key) const;

	/**
	 * @brief Executes a key up/down commands for a given key/modifier combination
	 */
	bool execute(int32_t key, int16_t modifier, bool pressed, double nowSeconds, uint16_t count = 1u);
};

inline bool KeyBindingHandler::isPressed(int32_t key) const {
	return _keys.hasKey(key);
}

inline const BindMap KeyBindingHandler::bindings() const {
	return _bindings;
}

}
