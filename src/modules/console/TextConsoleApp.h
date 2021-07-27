/**
 * @file
 */

#include "app/CommandlineApp.h"
#include "TextConsole.h"

namespace console {

/**
 * This is an application with an interactive text console to enter commands and change cvars
 * @sa command::Command
 * @sa core::Var
 */
class TextConsoleApp : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;

	TextConsole _console;
public:
	TextConsoleApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~TextConsoleApp();

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;
};

}
