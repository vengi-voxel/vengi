/**
 * @file
 */

#include "app/App.h"
#include "core/EventBus.h"
#include "core/TimeProvider.h"
#include "io/Filesystem.h"
#include "metric/Metric.h"
#include <gtest/gtest.h>

namespace app {

class TestApp : public App {
public:
	TestApp()
		: App(std::make_shared<metric::Metric>(), std::make_shared<io::Filesystem>(),
			  std::make_shared<core::EventBus>(), std::make_shared<core::TimeProvider>()) {
	}
};

TEST(AppTest, testLifecycleManual) {
	TestApp app;
	ASSERT_EQ(app::AppState::Init, app.onConstruct());
	ASSERT_EQ(app::AppState::Running, app.onInit());
	ASSERT_EQ(app::AppState::Cleanup, app.onRunning());
	ASSERT_EQ(app::AppState::Destroy, app.onCleanup());
}

TEST(AppTest, testLifecycleOnFrame) {
	TestApp app;
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
