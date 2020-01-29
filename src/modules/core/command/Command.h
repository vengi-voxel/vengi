/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <memory>
#include <utility>
#include <functional>
#include <unordered_map>
#include "core/Common.h"
#include "core/StringUtil.h"
#include "core/ReadWriteLock.h"
#include "ActionButton.h"

namespace core {

typedef std::vector<std::string> CmdArgs;

struct ActionButtonCommands {
	const std::string first;
	const std::string second;

	inline ActionButtonCommands(std::string&& _first, std::string&& _second) :
			first(_first), second(_second) {
	}

	/**
	 * @sa core::setBindingContext()
	 */
	ActionButtonCommands& setBindingContext(int context);
	ActionButtonCommands& setHelp(const char* help);
};

/**
 * @brief A command is a string bound c++ function/method/lambda. You can bind
 * this to keys or execute on the console.
 */
class Command {
private:
	typedef std::unordered_map<std::string, Command> CommandMap;
	typedef std::function<void(const CmdArgs&)> FunctionType;

	static CommandMap _cmds;
	static ReadWriteLock _lock;

	static uint64_t _delayMillis;
	static std::vector<std::string> _delayedTokens;

	std::string _name;
	const char* _help;
	FunctionType _func;
	BindingContext _bindingContext = BindingContext::All;
	typedef std::function<int(const std::string&, std::vector<std::string>& matches)> CompleteFunctionType;
	mutable CompleteFunctionType _completer;

	Command() :
		_name(""), _help(nullptr), _func() {
	}

	Command(const std::string& name, FunctionType&& func) :
		_name(name), _help(""), _func(std::move(func)) {
	}

public:
	static Command& registerCommand(const char* name, std::function<void(void)>& func) {
		return registerCommand(name, FunctionType([&] (const CmdArgs&) {func();}));
	}
	static Command& registerCommand(const char* name, FunctionType&& func);
	static bool unregisterCommand(const char* name);

	/**
	 * @brief Registers two commands prefixed with @c + and @c - (for pressed and released)
	 * for commands that are bound to keys.
	 * @param[in] name The name of the command. It will automatically be prefixed with
	 * a @c + and @c
	 * @param[in,out] button The @c ActionButton instance.
	 * @note This class is not taking ownership of the button instance. You have to ensure
	 * that the instance given here is alive as long as the commands are bound.
	 */
	static ActionButtonCommands registerActionButton(const std::string& name, ActionButton& button);
	static bool unregisterActionButton(const std::string& name);

	static void shutdown();

	/**
	 * @brief Executes delayed (by wait command e.g.) commands that are still in the command buffer
	 */
	static int update(uint64_t dt);

	static int execute(const std::string& command);

	static int execute(CORE_FORMAT_STRING const char* msg, ...) __attribute__((format(printf, 1, 2)));

	static bool execute(const std::string& command, const CmdArgs& args);
	static bool isSuitableBindingContext(BindingContext context);

	static Command* getCommand(const std::string& name) {
		auto i = _cmds.find(name);
		if (i == _cmds.end()) {
			return nullptr;
		}
		return &i->second;
	}

	template<class Functor>
	static void visit(Functor&& func) {
		CommandMap commandList;
		{
			ScopedReadLock lock(_lock);
			commandList = _cmds;
		}
		for (auto i = commandList.begin(); i != commandList.end(); ++i) {
			func(i->second);
		}
	}

	template<class Functor>
	static void visitSorted(Functor&& func) {
		std::vector<Command> commandList;
		{
			ScopedReadLock lock(_lock);
			commandList.reserve(_cmds.size());
			for (auto i = _cmds.begin(); i != _cmds.end(); ++i) {
				commandList.push_back(i->second);
			}
		}
		std::sort(commandList.begin(), commandList.end(), [] (const Command &v1, const Command &v2) {
			return strcmp(v1.name(), v2.name()) < 0;
		});
		for (const auto& command : commandList) {
			func(command);
		}
	}

	int complete(const std::string& str, std::vector<std::string>& matches) const;

	/**
	 * @param func A functor or lambda that accepts the following parameters: @code const std::string& str, std::vector<std::string>& matches @endcode
	 */
	template<class Functor>
	Command& setArgumentCompleter(Functor&& func) {
		_completer = func;
		return *this;
	}

	Command& setBoolCompleter();

	const char* name() const;

	Command& setHelp(const char* help);
	const char* help() const;

	Command& setBindingContext(int bindingContext);

	bool operator==(const Command& rhs) const;
};

inline bool Command::operator==(const Command& rhs) const {
	return rhs._name == _name;
}

inline Command& Command::setBindingContext(int bindingContext) {
	_bindingContext = (BindingContext)bindingContext;
	return *this;
}

inline const char* Command::name() const {
	return _name.c_str();
}

inline const char* Command::help() const {
	return _help;
}

}

namespace std {
template<> struct hash<core::Command> {
	inline size_t operator()(const core::Command &c) const {
		size_t result = 0;
		const size_t prime = 31;
		const char *name = c.name();
		const size_t s = strlen(name);
		for (size_t i = 0; i < s; ++i) {
			result = name[i] + (result * prime);
		}
		return result;
	}
};
}
