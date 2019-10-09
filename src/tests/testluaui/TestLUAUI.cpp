/**
 * @file
 */
#include "TestLUAUI.h"
#include "core/io/Filesystem.h"
#include "video/TexturePool.h"
#include "core/TimeProvider.h"
#include "core/EventBus.h"

TestLUAUI::TestLUAUI(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, const video::TexturePoolPtr& texturePool) :
		Super(metric, filesystem, eventBus, timeProvider, texturePool) {
	init(ORGANISATION, "testluaui");
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const video::TexturePoolPtr& texturePool = std::make_shared<video::TexturePool>(filesystem);
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	TestLUAUI app(metric, filesystem, eventBus, timeProvider, texturePool);
	return app.startMainLoop(argc, argv);
}
