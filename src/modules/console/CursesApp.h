/**
 * @file
 */

#include "app/CommandlineApp.h"
#include "CursesConsole.h"

namespace console {

class CursesApp : public core::CommandlineApp {
private:
	using Super = core::CommandlineApp;

	CursesConsole _console;
public:
	CursesApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~CursesApp();

	core::AppState onConstruct() override;
	core::AppState onInit() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
};

}
