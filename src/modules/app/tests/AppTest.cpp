/**
 * @file
 */

#include <gtest/gtest.h>
#include "app/App.h"
#include "metric/Metric.h"
#include "io/Filesystem.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"

namespace app {

TEST(AppTest, testLifecycleManual) {
	const metric::MetricPtr metric = std::make_shared<metric::Metric>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	App app(metric, filesystem, eventBus, timeProvider);
	ASSERT_EQ(app::AppState::Init, app.onConstruct());
	ASSERT_EQ(app::AppState::Running, app.onInit());
	ASSERT_EQ(app::AppState::Cleanup, app.onRunning());
	ASSERT_EQ(app::AppState::Destroy, app.onCleanup());
}

TEST(AppTest, testLifecycleOnFrame) {
	const metric::MetricPtr metric = std::make_shared<metric::Metric>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	App app(metric, filesystem, eventBus, timeProvider);
	app.onFrame();
	ASSERT_EQ(app::AppState::Construct, app.state());
	app.onFrame();
	ASSERT_EQ(app::AppState::Init, app.state());
	app.onFrame();
	ASSERT_EQ(app::AppState::Running, app.state());
	app.onFrame();
	ASSERT_EQ(app::AppState::Cleanup, app.state());
	app.onFrame();
	ASSERT_EQ(app::AppState::InvalidAppState, app.state());
}

}
