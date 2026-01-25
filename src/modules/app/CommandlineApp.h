/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/TimeProvider.h"

namespace app {

/**
 * Base application class that handles command line arguments, but doesn't support console input
 */
class CommandlineApp : public app::App {
private:
	using Super = app::App;
public:
	CommandlineApp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~CommandlineApp();
	int terminalWidth();
};

}

#define CONSOLE_APP(consoleAppName) \
int main(int argc, char *argv[]) { \
	const io::FilesystemPtr& filesystem = core::make_shared<io::Filesystem>(); \
	const core::TimeProviderPtr& timeProvider = core::make_shared<core::TimeProvider>(); \
	consoleAppName app(filesystem, timeProvider); \
	return app.startMainLoop(argc, argv); \
}
