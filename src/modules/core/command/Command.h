/**
 * @file
 */

#pragma once

#include <string>
#include <memory>
#include <utility>
#include <functional>
#include <unordered_map>
#include "core/String.h"
#include "core/ReadWriteLock.h"

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

	static int _delayFrames;
	static std::vector<std::string> _delayedTokens;

	const char* _name;
	const char* _help;
	FunctionType _func;
	typedef std::function<int(const std::string&, std::vector<std::string>& matches)> CompleteFunctionType;
	mutable CompleteFunctionType _completer;

	Command() :
		_name(nullptr), _help(nullptr), _func() {
	}

	Command(const char* name, FunctionType&& func) :
		_name(name), _help(""), _func(std::move(func)) {
	}

public:
	static Command& registerCommand(const char* name, FunctionType&& func) {
		ScopedWriteLock lock(_lock);
		const Command c(name, std::forward<FunctionType>(func));
		_cmds.insert(std::make_pair(name, c));
		return _cmds.find(name)->second;
	}

	static bool unregisterCommand(const char* name);

	static void shutdown();

	/**
	 * @brief Executes delayed (by wait command e.g.) commands that are still in the command buffer
	 */
	static int executeDelayed();

	static int execute(const std::string& command);

	static bool execute(const std::string& command, const CmdArgs& args);

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

	bool operator==(const Command& rhs) const;
};

inline bool Command::operator==(const Command& rhs) const {
	return rhs._name == _name;
}

inline const char* Command::name() const {
	return _name;
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
