/**
 * @file
 */

#include "CommandLock.h"
#include "Command.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Atomic.h"
#include "core/concurrent/Lock.h"
#include "core/concurrent/Thread.h"
#include <SDL_timer.h>

namespace command {

static core::AtomicInt _commandOwner{0};
static thread_local int _commandDepth = 0;

core_trace_mutex(core::Lock, _deferLock, "CommandDefer");
static core::DynamicArray<core::String> _deferredCommands;

bool commandExecutionAllowed() {
	int owner = (int)_commandOwner;
	if (owner == 0) {
		return true;
	}
	int tid = (int)core::getCurrentThreadId();
	return owner == tid;
}

void deferCommand(const core::String &commandLine) {
	core::ScopedLock lock(_deferLock);
	_deferredCommands.push_back(commandLine);
}

int drainDeferredCommands() {
	core::DynamicArray<core::String> commands;
	{
		core::ScopedLock lock(_deferLock);
		if (_deferredCommands.empty()) {
			return 0;
		}
		commands = core::move(_deferredCommands);
		_deferredCommands.clear();
	}
	int executed = 0;
	for (const core::String &cmd : commands) {
		// These are re-dispatched via Command::execute which will re-check the gate
		// but at this point no async command should be running
		executed += Command::execute(cmd);
	}
	return executed;
}

ScopedCommandOwnership::ScopedCommandOwnership() {
	int tid = (int)core::getCurrentThreadId();
	int expected = 0;
	while (!_commandOwner.compare_exchange(expected, tid)) {
		expected = 0;
		SDL_Delay(1);
	}
	_commandDepth = 1;
}

ScopedCommandOwnership::~ScopedCommandOwnership() {
	_commandDepth = 0;
	_commandOwner = 0;
}

} // namespace command
