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
		TestApp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider, AbstractTest* test, size_t threadPoolSize = 1);
		virtual ~TestApp();

		void run();
		bool createPid() override {
			return false;
		}
		app::AppState onInit() override;
		app::AppState onCleanup() override;
		app::AppState onRunning() override;
	};

protected:
	TestApp *_testApp = nullptr;
	size_t _threadPoolSize;

	core::String fileToString(const core::String& filename) const;

	virtual void onCleanupApp() {
	}

	virtual bool onInitApp() {
		return true;
	}

public:
	AbstractTest(size_t threadPoolSize = 1) : _threadPoolSize(threadPoolSize) {
	}

	virtual void SetUp() override;

	virtual void TearDown() override;
};

}
