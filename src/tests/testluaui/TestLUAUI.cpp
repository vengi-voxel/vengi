/**
 * @file
 */
#include "TestLUAUI.h"
#include "io/Filesystem.h"
#include "video/TexturePool.h"
#include "video/TextureAtlasRenderer.h"
#include "core/TimeProvider.h"
#include "core/EventBus.h"
#include "metric/Metric.h"
#include "voxelformat/MeshCache.h"
#include "voxelrender/CachedMeshRenderer.h"

TestLUAUI::TestLUAUI(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const video::TexturePoolPtr& texturePool,
		const voxelrender::CachedMeshRendererPtr& meshRenderer,
		const video::TextureAtlasRendererPtr& textureAtlasRenderer) :
		Super(metric, filesystem, eventBus, timeProvider, texturePool, meshRenderer, textureAtlasRenderer) {
	init(ORGANISATION, "testluaui");
	pushWindow("overview");
	pushWindow("calculator");
	pushWindow("stylewin");
	pushWindow("modelwin");
}

int main(int argc, char *argv[]) {
	const voxelformat::MeshCachePtr& meshCache = std::make_shared<voxelformat::MeshCache>();
	const voxelrender::CachedMeshRendererPtr& meshRenderer = core::make_shared<voxelrender::CachedMeshRenderer>(meshCache);
	const video::TextureAtlasRendererPtr& textureAtlasRenderer = core::make_shared<video::TextureAtlasRenderer>();
	const core::EventBusPtr& eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr& filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr& timeProvider = std::make_shared<core::TimeProvider>();
	const video::TexturePoolPtr& texturePool = std::make_shared<video::TexturePool>(filesystem);
	const metric::MetricPtr& metric = std::make_shared<metric::Metric>();
	TestLUAUI app(metric, filesystem, eventBus, timeProvider, texturePool, meshRenderer, textureAtlasRenderer);
	return app.startMainLoop(argc, argv);
}
