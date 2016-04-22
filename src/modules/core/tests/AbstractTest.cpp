#include "AbstractTest.h"
#include "core/Log.h"

namespace core {

void AbstractTest::SetUp() {
	Log::init();
	const core::EventBusPtr eventBus(new core::EventBus());
	const io::FilesystemPtr filesystem(new io::Filesystem());
	_testApp = new TestApp(filesystem, eventBus);
}

void AbstractTest::TearDown() {
	delete _testApp;
}

}
