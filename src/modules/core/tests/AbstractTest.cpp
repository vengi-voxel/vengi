/**
 * @file
 */

#include "AbstractTest.h"
#include "core/Log.h"
#include "core/Var.h"

namespace core {

void AbstractTest::SetUp() {
	core::Var::get(cfg::CoreLogLevel, std::to_string(SDL_LOG_PRIORITY_WARN));
	Log::init();
	const core::EventBusPtr eventBus(new core::EventBus());
	const io::FilesystemPtr filesystem(new io::Filesystem());
	_testApp = new TestApp(filesystem, eventBus);
}

void AbstractTest::TearDown() {
	delete _testApp;
	Var::shutdown();
}

}
