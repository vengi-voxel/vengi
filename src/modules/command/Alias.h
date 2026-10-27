/**
 * @file
 */

#pragma once

#include "core/Pair.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "core/concurrent/Lock.h"

namespace command {

/**
 * @brief Named shortcuts for one or more commands.
 *
 * Aliases are registered as commands, so they can be executed from the console
 * and bound to keys. They are persisted next to the cvars.
 *
 * @sa command::Command
 */
class Alias {
private:
	typedef core::StringMap<core::String> AliasMap;
	static AliasMap _aliases core_thread_guarded_by(_lock);
	static core::Lock _lock;

	static bool isValidName(const core::String &name);

public:
	/**
	 * @brief Register the @c alias and @c unalias commands
	 */
	static void construct();
	static void shutdown();

	/**
	 * @brief Create or update an alias and register it as a command
	 * @return @c false if the name is invalid or already used by a non-alias command or cvar
	 */
	static bool set(const core::String &name, const core::String &commands);
	/**
	 * @brief Remove an alias and unregister the command
	 */
	static bool remove(const core::String &name);
	static bool has(const core::String &name);
	/**
	 * @return The command string, or an empty string if the alias does not exist
	 */
	static core::String get(const core::String &name);
	static void execute(const core::String &name);
	static size_t size();

	static core::String serialize();
	static bool parse(const core::String &content);

	template<class Functor>
	static void visit(Functor &&func) {
		core::ScopedLock lock(_lock);
		for (auto i = _aliases.begin(); i != _aliases.end(); ++i) {
			func(i->key, i->value);
		}
	}

	template<class Functor>
	static void visitSorted(Functor &&func) {
		core::DynamicArray<core::Pair<core::String, core::String>> entries;
		visit([&](const core::String &name, const core::String &commands) {
			entries.emplace_back(name, commands);
		});
		entries.sort([](const core::Pair<core::String, core::String> &a, const core::Pair<core::String, core::String> &b) {
			return a.first > b.first;
		});
		for (const core::Pair<core::String, core::String> &entry : entries) {
			func(entry.first, entry.second);
		}
	}
};

} // namespace command
