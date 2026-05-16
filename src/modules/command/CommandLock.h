/**
 * @file
 * @brief Command ownership gate to prevent concurrent command execution
 *
 * When an async command is running, commands from other threads (UI, keybinds, lua)
 * are deferred and drained on the next Command::update() call.
 */

#pragma once

#include "core/String.h"

namespace command {

/**
 * @brief Check whether the current thread is allowed to execute commands
 * @return true if execution is allowed, false if the command was deferred
 */
bool commandExecutionAllowed();

/**
 * @brief Defer a command for later execution
 */
void deferCommand(const core::String &commandLine);

/**
 * @brief Drain all deferred commands - called from Command::update on the main thread
 * @return number of commands executed
 */
int drainDeferredCommands();

/**
 * @brief RAII guard that claims command ownership for the current thread.
 * Spins until ownership is acquired. Used in executeCommandsAsync.
 */
class ScopedCommandOwnership {
public:
	ScopedCommandOwnership();
	~ScopedCommandOwnership();

	ScopedCommandOwnership(const ScopedCommandOwnership &) = delete;
	ScopedCommandOwnership &operator=(const ScopedCommandOwnership &) = delete;
};

} // namespace command
