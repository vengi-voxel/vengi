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


AppState CommandlineApp::onConstruct() {
	const app::AppState state = Super::onConstruct();

	registerArg("--trace").setDescription("Change log level to trace");
	registerArg("--debug").setDescription("Change log level to debug");
	registerArg("--info").setDescription("Change log level to info");
	registerArg("--warn").setDescription("Change log level to warn");
	registerArg("--error").setDescription("Change log level to error");

	bool changed = false;
	if (hasArg("--trace")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal((int)Log::Level::Trace);
		changed = true;
	} else if (hasArg("--debug")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal((int)Log::Level::Debug);
		changed = true;
	} else if (hasArg("--info")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal((int)Log::Level::Info);
		changed = true;
	} else if (hasArg("--warn")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal((int)Log::Level::Warn);
		changed = true;
	} else if (hasArg("--error")) {
		core::Var::getSafe(cfg::CoreLogLevel)->setVal((int)Log::Level::Error);
		changed = true;
	}
	if (changed) {
		Log::init();
	}
	return state;
}

}
