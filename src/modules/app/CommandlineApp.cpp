/**
 * @file
 */

#include "CommandlineApp.h"
#include "core/Var.h"
#include "core/Log.h"
#include <SDL_platform.h>
#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x4
#endif
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace app {

CommandlineApp::CommandlineApp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize) :
		Super(filesystem, timeProvider, threadPoolSize) {
#if defined(_WIN32) || defined(__CYGWIN__)
	// https://learn.microsoft.com/en-us/windows/console/setconsolemode
	HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	if (stdoutHandle != INVALID_HANDLE_VALUE) {
		DWORD console_mode;
		if (GetConsoleMode(stdoutHandle, &console_mode)) {
			SetConsoleMode(stdoutHandle, ENABLE_VIRTUAL_TERMINAL_PROCESSING | console_mode);
		}
	}
#endif
}

CommandlineApp::~CommandlineApp() {
}

int CommandlineApp::terminalWidth() {
#if defined(_WIN32) || defined(__CYGWIN__)
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		return csbi.srWindow.Right - csbi.srWindow.Left + 1;
	}
#else
	struct winsize w;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
		return w.ws_col;
	}
#endif
	return -1; // fallback or error
}

}
