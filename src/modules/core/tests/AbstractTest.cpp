#include "AbstractTest.h"
#include "core/Log.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace core {

void AbstractTest::SetUp() {
	Log::init();
	core::EventBusPtr eventBus = core::EventBusPtr(new core::EventBus());
	io::FilesystemPtr filesystem = io::FilesystemPtr(new io::Filesystem());
	_testApp = new TestApp(filesystem,eventBus);
}

void AbstractTest::TearDown() {
	delete _testApp;
}

}
