/**
 * @file
 */

#include "app/CommandlineApp.h"
#include "TextConsole.h"

namespace console {

class CursesApp : public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;

	TextConsole _console;
public:
	CursesApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~CursesApp();

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;
};

}
