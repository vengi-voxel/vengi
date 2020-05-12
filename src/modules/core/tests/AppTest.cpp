/**
 * @file
 */

#include <gtest/gtest.h>
#include "core/App.h"
#include "core/metric/Metric.h"
#include "core/io/Filesystem.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"

namespace core {

TEST(AppTest, testLifecycleManual) {
	const metric::MetricPtr metric = std::make_shared<metric::Metric>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	App app(metric, filesystem, eventBus, timeProvider);
	ASSERT_EQ(core::AppState::Init, app.onConstruct());
	ASSERT_EQ(core::AppState::Running, app.onInit());
	ASSERT_EQ(core::AppState::Cleanup, app.onRunning());
	ASSERT_EQ(core::AppState::Destroy, app.onCleanup());
}

TEST(AppTest, testLifecycleOnFrame) {
	const metric::MetricPtr metric = std::make_shared<metric::Metric>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	App app(metric, filesystem, eventBus, timeProvider);
	app.onFrame();
	ASSERT_EQ(core::AppState::Construct, app.state());
	app.onFrame();
	ASSERT_EQ(core::AppState::Init, app.state());
	app.onFrame();
	ASSERT_EQ(core::AppState::Running, app.state());
	app.onFrame();
	ASSERT_EQ(core::AppState::Cleanup, app.state());
	app.onFrame();
	ASSERT_EQ(core::AppState::InvalidAppState, app.state());
}

}