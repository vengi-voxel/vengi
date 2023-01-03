/**
 * @file
 */

#pragma once

#include "core/BindingContext.h"
#include "core/Tokenizer.h"
#include <unordered_map>

namespace util {

struct CommandModifierPair {
	inline CommandModifierPair(const core::String& _command, int16_t _modifier, uint16_t _count, core::BindingContext _context) :
			command(_command), modifier(_modifier), count(_count), context(_context) {
	}
	core::String command;
	int16_t modifier;
	uint16_t count;
	core::BindingContext context;
};
typedef std::unordered_multimap<int32_t, CommandModifierPair> BindMap;

/**
 * @brief Parses keys/command combinations
 */
class KeybindingParser {
private:
	BindMap _bindings;
	int _invalidBindings;

	void parseKeyAndCommand(core::String key, const core::String& command, const core::String &context);
public:
	/**
	 * @brief Parses a single binding
	 */
	KeybindingParser(const core::String& key, const core::String& binding, const core::String &context);

	/**
	 * @brief Parses a buffer of bindings. Each binding is separated by a newline.
	 */
	KeybindingParser(const core::String& bindings);

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
