/**
 * @file
 */

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <unordered_map>
#include "String.h"
#include "ReadWriteLock.h"

namespace core {

typedef std::vector<std::string> CmdArgs;

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

	std::string _name;
	std::string _help;
	FunctionType _func;
	typedef std::function<int(const std::string&, std::vector<std::string>& matches)> CompleteFunctionType;
	mutable CompleteFunctionType _completer;

	Command(std::string&& name, FunctionType&& func) :
			_name(std::move(name)), _func(std::move(func)) {
	}

public:
	static Command& registerCommand(std::string&& name, FunctionType&& func) {
		ScopedWriteLock lock(_lock);
		const Command c(std::move(name), std::move(func));
		_cmds.insert(std::make_pair(c.name(), c));
		return _cmds.find(c.name())->second;
	}

	template<class Function>
	static Command& registerCommand2(std::string&& name, Function&& func) {
		return registerCommand(std::move(name), std::bind(std::forward<Function>(func)));
	}

	static void unregisterCommand(const std::string& name) {
		ScopedWriteLock lock(_lock);
		_cmds.erase(name);
	}

	static int execute(const std::string& command) {
		std::vector<std::string> commands;
		core::string::splitString(command, commands, ";");
		int executed = 0;
		for (const std::string& c : commands) {
			std::vector<std::string> args;
			core::string::splitString(c, args);
			if (args.empty())
				continue;
			const std::string cmd = core::string::eraseAllSpaces(args[0]);
			args.erase(args.begin());
			if (execute(cmd, args))
				++executed;
		}
		return executed;
	}

	static bool execute(const std::string& command, const CmdArgs& args) {
		ScopedReadLock lock(_lock);
		auto i = _cmds.find(command);
		if (i == _cmds.end())
			return false;
		const Command& cmd = i->second;
		cmd._func(args);
		return true;
	}

	template<class Functor>
	static void visit(Functor&& func) {
		ScopedReadLock lock(_lock);
		for (auto i = _cmds.begin(); i != _cmds.end(); ++i) {
			func(i->second);
		}
	}

	template<class Functor>
	static void visitSorted(Functor&& func) {
		ScopedReadLock lock(_lock);
		std::vector<Command> commandList;
		for (auto i = _cmds.begin(); i != _cmds.end(); ++i) {
			commandList.push_back(i->second);
		}
		std::sort(begin(commandList), end(commandList), [](auto const &v1, auto const &v2) {
			return v1.name() < v2.name();
		});
		for (const auto& command : commandList) {
			func(command);
		}
	}

	int complete(const std::string& str, std::vector<std::string>& matches) const;

	Command& setHelp(const std::string& help) {
		_help = help;
		return *this;
	}

	/**
	 * @param func A functor or lambda that accepts the following parameters: @code const std::string& str, std::vector<std::string>& matches @endcode
	 */
	template<class Functor>
	Command& setArgumentCompleter(Functor&& func) {
		_completer = func;
		return *this;
	}

	Command& setBoolCompleter() {
		return setArgumentCompleter([] (const std::string& str, std::vector<std::string>& matches) -> int {
			if (str[0] == 't') {
				matches.push_back("true");
				return 1;
			}
			if (str[0] == 'f') {
				matches.push_back("false");
				return 1;
			}
			matches.push_back("true");
			matches.push_back("false");
			return 2;
		});
	}

	inline bool operator==(const Command& rhs) const {
		return rhs._name == _name;
	}

	inline const std::string& name() const {
		return _name;
	}

	inline const std::string& help() const {
		return _help;
	}
};

}

namespace std {
template<> struct hash<core::Command> {
	inline size_t operator()(const core::Command &c) const {
		return std::hash<std::string>()(c.name());
	}
};
}
