/**
 * @file
 */

#pragma once

#include "core/tests/TestHelper.h"
#include "app/CommandlineApp.h"

namespace app {

class AbstractTest: public testing::Test {
private:
	class TestApp: public app::CommandlineApp {
		friend class AbstractTest;
	private:
		using Super = app::CommandlineApp;
	protected:
		AbstractTest* _test = nullptr;
	public:
		TestApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, AbstractTest* test);
		virtual ~TestApp();

		app::AppState onInit() override;
		app::AppState onCleanup() override;
	};

protected:
	TestApp *_testApp = nullptr;

	core::String fileToString(const core::String& filename) const;

	virtual void onCleanupApp() {
	}

	virtual bool onInitApp() {
		return true;
	}

public:
	virtual void SetUp() override;

	virtual void TearDown() override;
};

}
