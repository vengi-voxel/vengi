/**
 * @file
 */

#include "Process.h"
#include <SDL_platform.h>
#include <SDL_assert.h>
#include "core/Log.h"

#include <fcntl.h>
#include <string.h>

#if defined(__WINDOWS__)
#include <SDL.h>
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#elif defined(__LINUX__) or defined(__MACOSX__)
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dlfcn.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>
#endif

namespace core {

int Process::exec(const std::string& command, const std::vector<std::string>& arguments, const char* workingDirectory, size_t bufSize, char *output) {
#if defined(__LINUX__) || defined(__MACOSX__)
	int link[2];
	if (::pipe(link) < 0) {
		Log::error("pipe failed: %s", strerror(errno));
		return -1;
	}
	const pid_t childPid = ::fork();
	if (childPid < 0) {
		Log::error("fork failed: %s", strerror(errno));
		return -1;
	}

	if (childPid == 0) {
		const char* argv[64];
		int argc = 0;
		argv[argc++] = command.c_str();
		for (const std::string& arg : arguments) {
			argv[argc++] = arg.c_str();
			if (argc >= 63) {
				break;
			}
		}
		argv[argc] = nullptr;
		::dup2(link[1], STDOUT_FILENO);
		::close(link[0]);
		::close(link[1]);
		if (workingDirectory != nullptr) {
			const int retVal = ::chdir(workingDirectory);
			if (retVal == -1) {
				Log::warn("Could not change current working dir to %s", workingDirectory);
			}
		}
		// we are the child
		::execv(command.c_str(), const_cast<char* const*>(argv));

		// this should never get called
		Log::error("failed to run '%s' with %i parameters: %s (%i)", command.c_str(), (int)arguments.size(), strerror(errno), errno);
		::exit(-1);
	}

	close(link[1]);
	if (bufSize > 0) {
		char* p = output;
		int size = bufSize;
		for (;;) {
			const int n = ::read(link[0], p, size);
			if (n <= 0) {
				break;
			}
			p += n;
			size -= n;
		}
	}
	// we are the parent and are blocking until the child stopped
	int status;
	const pid_t pid = ::wait(&status);
#ifdef DEBUG
	SDL_assert(pid == childPid);
#else
	(void)pid;
#endif

	// check for success
	if (!WIFEXITED(status)) {
		Log::warn("child process exists with error");
		return -1;
	}

	Log::debug("child process returned with code %d", WEXITSTATUS(status));
	if (WEXITSTATUS(status) != 0) {
		return -1;
	}

	// success
	return 0;
#elif __WINDOWS__
	std::string cmd = command;
	if (!arguments.empty()) {
		cmd.append(" ");
	}
	for (const std::string& argument : arguments) {
		cmd.append(argument);
		cmd.append(" ");
	}

	STARTUPINFO startupInfo;
	PROCESS_INFORMATION processInfo;

	char* commandPtr = SDL_strdup(cmd.c_str());
	if (!CreateProcess(nullptr, (LPSTR) commandPtr, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, NULL, nullptr,
			&startupInfo, &processInfo)) {
		SDL_free(commandPtr);
		return -1;
	}
	SDL_free(commandPtr);

	WaitForSingleObject(processInfo.hProcess, INFINITE);

	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);

	return 0;
#else
	Log::error("Process::exec is not implemented for this platform");
	return 1;
#endif
}

}
