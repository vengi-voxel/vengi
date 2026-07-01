/**
 * @file
 */

#include "Process.h"
#include "core/Log.h"
#include "io/Stream.h"
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_process.h>
#include <SDL3/SDL_properties.h>
#if defined(_WIN32) || defined(__CYGWIN__)
#include <direct.h>
#else
#include <unistd.h>
#endif

namespace core {

int Process::exec(const core::String &command, const core::DynamicArray<core::String> &arguments,
				  const char *workingDirectory, io::WriteStream *stream) {
	core::DynamicArray<const char *> argv;
	argv.reserve(arguments.size() + 2);
	argv.push_back(command.c_str());
	for (const core::String &arg : arguments) {
		argv.push_back(arg.c_str());
	}
	argv.push_back(nullptr);

	const SDL_PropertiesID props = SDL_CreateProperties();
	if (props == 0) {
		Log::error("Failed to create SDL process properties: %s", SDL_GetError());
		return -1;
	}

	SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, (void *)argv.data());
	char *oldWorkingDirectory = nullptr;
	// SDL 3.4.0
#if !defined(SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING)
	bool restoreWorkingDirectory = false;
#endif
	if (workingDirectory != nullptr && workingDirectory[0] != '\0') {
#ifdef SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING
		SDL_SetStringProperty(props, SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING, workingDirectory);
#else
		oldWorkingDirectory = SDL_GetCurrentDirectory();
		if (oldWorkingDirectory != nullptr) {
#if defined(_WIN32) || defined(__CYGWIN__)
			if (_chdir(workingDirectory) != 0) {
#else
			if (chdir(workingDirectory) != 0) {
#endif
				Log::warn("Failed to change process working directory to '%s'", workingDirectory);
			} else {
				restoreWorkingDirectory = true;
			}
		} else {
			Log::warn("Failed to query current working directory: %s", SDL_GetError());
		}
#endif
	}
	if (stream != nullptr) {
		SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, (Sint64)SDL_PROCESS_STDIO_APP);
		SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
	}

	SDL_Process *process = SDL_CreateProcessWithProperties(props);
	SDL_DestroyProperties(props);
	if (process == nullptr) {
		Log::error("Failed to create process '%s': %s", command.c_str(), SDL_GetError());
#if !defined(SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING)
		if (restoreWorkingDirectory && oldWorkingDirectory != nullptr) {
#if defined(_WIN32) || defined(__CYGWIN__)
			_chdir(oldWorkingDirectory);
#else
			chdir(oldWorkingDirectory);
#endif
		}
		SDL_free(oldWorkingDirectory);
#endif
		return -1;
	}
#if !defined(SDL_PROP_PROCESS_CREATE_WORKING_DIRECTORY_STRING)
	if (restoreWorkingDirectory && oldWorkingDirectory != nullptr) {
#if defined(_WIN32) || defined(__CYGWIN__)
		if (_chdir(oldWorkingDirectory) != 0) {
#else
		if (chdir(oldWorkingDirectory) != 0) {
#endif
			Log::warn("Failed to restore working directory to '%s'", oldWorkingDirectory);
		}
	}
	SDL_free(oldWorkingDirectory);
#endif

	int exitCode = -1;
	if (stream != nullptr) {
		size_t outputSize = 0u;
		void *output = SDL_ReadProcess(process, &outputSize, &exitCode);
		if (output == nullptr) {
			Log::error("Failed to read process output for '%s': %s", command.c_str(), SDL_GetError());
			SDL_DestroyProcess(process);
			return -1;
		}
		if (outputSize > 0u) {
			stream->write(output, (int)outputSize);
		}
		SDL_free(output);
	} else {
		if (!SDL_WaitProcess(process, true, &exitCode)) {
			Log::error("Failed to wait for process '%s': %s", command.c_str(), SDL_GetError());
			SDL_DestroyProcess(process);
			return -1;
		}
	}

	SDL_DestroyProcess(process);
	if (exitCode != 0) {
		Log::debug("child process returned with code %d", exitCode);
		return -1;
	}
	return 0;
}
} // namespace core
