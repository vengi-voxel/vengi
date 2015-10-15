#include <gtest/gtest.h>

#include "core/App.h"
#include "core/EventBus.h"
#include "io/Filesystem.h"

namespace core {

class AbstractTest: public testing::Test {
private:
	class TestApp: public core::App {
	public:
		TestApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus) :
				core::App(filesystem, eventBus, 10000) {
			init("engine", "test");
		}
	};

	TestApp *_testApp;

public:
	void SetUp() override {
		core::EventBusPtr eventBus = core::EventBusPtr(new core::EventBus());
		io::FilesystemPtr filesystem = io::FilesystemPtr(new io::Filesystem());
		_testApp = new TestApp(filesystem,eventBus);
	}

	void TearDown() override {
		delete _testApp;
	}
};

}
