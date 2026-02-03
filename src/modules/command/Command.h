/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Common.h"
#include "core/collection/StringMap.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Lock.h"
#include "command/ActionButton.h"
#include <functional>

namespace command {

#define COMMAND_PRESSED "+"
#define COMMAND_RELEASED "-"

/**
 * @brief Argument types for command parameters
 */
enum class ArgType : uint8_t {
	String,
	Int,
	Float,
	Bool
};

class Command;

/**
 * @brief Completion function type for individual arguments
 */
typedef std::function<int(const core::String&, core::DynamicArray<core::String>& matches)> ArgCompleterFunc;

/**
 * @brief Definition of a single command argument
 */
struct CommandArg {
	core::String name;
	core::String description;
	core::String defaultVal;
	ArgType type = ArgType::String;
	bool optional = false;
	ArgCompleterFunc completer;

	CommandArg() = default;
	CommandArg(const core::String &n, ArgType t = ArgType::String, bool opt = false,
			   const core::String &def = "", const core::String &desc = "")
		: name(n), description(desc), defaultVal(def), type(t), optional(opt) {
	}
};

/**
 * @brief Parsed command arguments with named access
 */
class CommandArgs {
private:
	core::StringMap<core::String> _values;

public:
	CommandArgs() = default;

	void set(const core::String &name, const core::String &value) {
		_values.put(name, value);
	}

	bool has(const core::String &name) const {
		return _values.find(name) != _values.end();
	}

	const core::String &str(const core::String &name) const {
		static const core::String empty;
		auto iter = _values.find(name);
		if (iter == _values.end()) {
			return empty;
		}
		return iter->value;
	}

	core::String str(const core::String &name, const core::String &defaultVal) const {
		auto iter = _values.find(name);
		if (iter == _values.end()) {
			return defaultVal;
		}
		return iter->value;
	}

	int intVal(const core::String &name, int defaultVal = 0) const {
		auto iter = _values.find(name);
		if (iter == _values.end()) {
			return defaultVal;
		}
		return iter->value.toInt();
	}

	float floatVal(const core::String &name, float defaultVal = 0.0f) const {
		auto iter = _values.find(name);
		if (iter == _values.end()) {
			return defaultVal;
		}
		return core::string::toFloat(iter->value);
	}

	bool boolVal(const core::String &name, bool defaultVal = false) const {
		auto iter = _values.find(name);
		if (iter == _values.end()) {
			return defaultVal;
		}
		const core::String &val = iter->value;
		return val == "true" || val == "1" || val == "yes" || val == "on";
	}

	size_t size() const {
		return _values.size();
	}

	bool empty() const {
		return _values.empty();
	}
};

struct ActionButtonCommands {
	const core::String first;
	const core::String second;

	inline ActionButtonCommands(core::String&& _first, core::String&& _second) :
			first(_first), second(_second) {
	}

	ActionButtonCommands& setHelp(const char* help);
};

#define MAX_COMMANDS 4096

/**
 * @brief A command is a string bound c++ function/method/lambda. You can bind
 * this to keys or execute on the console.
 */
class Command {
private:
	typedef core::StringMap<Command> CommandMap;
	typedef std::function<void(const CommandArgs&)> FunctionType;

	static CommandMap _cmds core_thread_guarded_by(_lock);
	static core::Lock _lock;
	static size_t _sortedCommandListSize;
	static Command* _sortedCommandList[MAX_COMMANDS];

	static double _delaySeconds;
	static core::DynamicArray<core::String> _delayedTokens;

	core::String _name;
	core::String _help;
	FunctionType _func;
	core::DynamicArray<CommandArg> _args;
	typedef std::function<int(const core::String&, core::DynamicArray<core::String>& matches)> CompleteFunctionType;
	mutable CompleteFunctionType _completer;

	Command() :
		_func() {
	}

	Command(const core::String& name) :
		_name(name), _func() {
	}

	static void updateSortedList();

	/**
	 * @brief Generate usage message based on argument definitions
	 */
	core::String usage() const;

	/**
	 * @brief Parse raw arguments into CommandArgs based on definitions
	 * @return true if parsing succeeded (all required args present), false otherwise
	 */
	bool parseArgs(const core::DynamicArray<core::String>& rawArgs, CommandArgs& out) const;

public:
	/**
	 * @brief Register a new command with the given name
	 * @return Reference to the command for chaining
	 */
	static Command& registerCommand(const core::String &name);
	static bool unregisterCommand(const core::String &name);

	/**
	 * @brief Add an argument definition to this command
	 * @param arg The argument definition
	 * @return Reference to this command for chaining
	 */
	Command& addArg(const CommandArg &arg);

	/**
	 * @brief Set the handler function for this command
	 * @param func The function to execute when the command is invoked
	 * @return Reference to this command for chaining
	 */
	template<class Functor>
	Command& setHandler(Functor&& func) {
		_func = std::forward<Functor>(func);
		return *this;
	}

	/**
	 * @brief Registers two commands prefixed with @c + and @c - (for pressed and released)
	 * for commands that are bound to keys.
	 * @param[in] name The name of the command. It will automatically be prefixed with
	 * a @c + and @c -
	 * @param[in,out] button The @c ActionButton instance.
	 * @note This class is not taking ownership of the button instance. You have to ensure
	 * that the instance given here is alive as long as the commands are bound.
	 */
	static ActionButtonCommands registerActionButton(const core::String& name, ActionButton& button, const core::String &help = "");
	static bool unregisterActionButton(const core::String& name);

	static void shutdown();

	/**
	 * @brief Executes delayed (by wait command e.g.) commands that are still in the command buffer
	 */
	static int update(double deltaFrameSeconds);

	static int execute(const core::String& command);

	static int execute(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(1);

	static bool execute(const core::String& command, const core::DynamicArray<core::String>& rawArgs);

	static Command* getCommand(const core::String& name) {
		core::ScopedLock lock(_lock);
		auto i = _cmds.find(name);
		if (i == _cmds.end()) {
			return nullptr;
		}
		return (Command*)&i->value;
	}

	template<class Functor>
	static void visit(Functor&& func) {
		core::ScopedLock lock(_lock);
		for (auto i = _cmds.begin(); i != _cmds.end(); ++i) {
			func(i->value);
		}
	}

	template<class Functor>
	static void visitSorted(Functor&& func) {
		updateSortedList();
		core::ScopedLock lock(_lock);
		for (size_t i = 0; i < _sortedCommandListSize; ++i) {
			func(*_sortedCommandList[i]);
		}
	}

	int complete(const core::String& str, core::DynamicArray<core::String>& matches) const;

	/**
	 * @brief Complete specific argument by index
	 * @param argIndex The index of the argument to complete (0-based)
	 * @param str The current input string for the argument
	 * @param matches Output array for completion matches
	 * @return Number of matches found
	 */
	int completeArg(int argIndex, const core::String& str, core::DynamicArray<core::String>& matches) const;

	/**
	 * @param func A functor or lambda that accepts the following parameters: @code const core::String& str, core::DynamicArray<core::String>& matches @endcode
	 */
	template<class Functor>
	Command& setArgumentCompleter(Functor&& func) {
		_completer = func;
		return *this;
	}

	Command& setBoolCompleter();

	bool isInput() const;

	const core::String &name() const;
	const core::DynamicArray<CommandArg> &args() const;

	Command& setHelp(const core::String &help);
	const core::String &help() const;

	bool operator==(const Command& rhs) const;
};

inline bool Command::operator==(const Command& rhs) const {
	return rhs._name == _name;
}

inline const core::String &Command::name() const {
	return _name;
}

inline const core::String &Command::help() const {
	return _help;
}

inline const core::DynamicArray<CommandArg> &Command::args() const {
	return _args;
}

inline core::String help(const core::String &cmd) {
	Command* command = Command::getCommand(cmd);
	if (!command) {
		return core::String::Empty;
	}
	return command->help();
}

}
