/**
 * @file
 */

#include "Template.h"
#include "app/App.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"
#include "io/Filesystem.h"
#include "metric/Metric.h"

Template::Template(const metric::MetricPtr &metric, const core::EventBusPtr &eventBus,
				   const core::TimeProviderPtr &timeProvider, const io::FilesystemPtr &filesystem,
			const tmpl::ExamplePtr &example)
	: Super(metric, filesystem, eventBus, timeProvider), _example(example) {
	init(ORGANISATION, "Template");
}

app::AppState Template::onConstruct() {
	const app::AppState state = Super::onConstruct();
	_example->construct();
	/** insert your code here */
	return state;
}

app::AppState Template::onInit() {
	const app::AppState state = Super::onInit();
	if (state == app::AppState::InitFailure) {
		return state;
	}
	if (!_example->init()) {
		Log::error("Failed to init example");
		return app::AppState::InitFailure;
	}
	/** insert your code here */
	return state;
}

app::AppState Template::onRunning() {
	const app::AppState state = Super::onRunning();
	if (state != app::AppState::Running) {
		return state;
	}
	_example->update();
	/** insert your code here */
	return state;
}

void Template::onRenderUI() {
	/** insert your code here */
}

app::AppState Template::onCleanup() {
	_example->shutdown();
	/** insert your code here */
	return Super::onCleanup();
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr &eventBus = std::make_shared<core::EventBus>();
	const core::TimeProviderPtr &timeProvider = std::make_shared<core::TimeProvider>();
	const io::FilesystemPtr &filesystem = std::make_shared<io::Filesystem>();
	const metric::MetricPtr &metric = std::make_shared<metric::Metric>();
	const tmpl::ExamplePtr& example = core::make_shared<tmpl::Example>();
	Template app(metric, eventBus, timeProvider, filesystem, example);
	return app.startMainLoop(argc, argv);
}
