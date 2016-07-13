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

	Command(const std::string& name, FunctionType&& func);

public:
	static Command& registerCommand(const std::string& name, FunctionType&& func);

	template<class Function>
	static Command& registerCommand2(const std::string& name, Function&& func) {
		return registerCommand(name, std::bind(std::forward<Function>(func)));
	}

	static void unregisterCommand(const std::string& name);

	static int execute(const std::string& command);

	static bool execute(const std::string& command, const CmdArgs& args);

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

	/**
	 * @param func A functor or lambda that accepts the following parameters: @code const std::string& str, std::vector<std::string>& matches @endcode
	 */
	template<class Functor>
	Command& setArgumentCompleter(Functor&& func) {
		_completer = func;
		return *this;
	}

	Command& setBoolCompleter();

	const std::string& name() const;

	Command& setHelp(const std::string& help);
	const std::string& help() const;

	bool operator==(const Command& rhs) const;
};

inline bool Command::operator==(const Command& rhs) const {
	return rhs._name == _name;
}

inline const std::string& Command::name() const {
	return _name;
}

inline const std::string& Command::help() const {
	return _help;
}

}

namespace std {
template<> struct hash<core::Command> {
	inline size_t operator()(const core::Command &c) const {
		return std::hash<std::string>()(c.name());
	}
};
}
