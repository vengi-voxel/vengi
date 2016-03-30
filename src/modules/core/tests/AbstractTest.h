#include <gtest/gtest.h>

#include "core/App.h"
#include "core/EventBus.h"
#include "io/Filesystem.h"

#include "stb_image_write.h"

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
	void SetUp() override;

	void TearDown() override;
};

}
