/**
 * @file
 */

#pragma once

#include "ui/nuklear/LUAUIApp.h"

class TestLUAUI: public ui::nuklear::LUAUIApp {
private:
	using Super = ui::nuklear::LUAUIApp;
public:
	TestLUAUI(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const video::TexturePoolPtr& texturePool,
		const voxelrender::CachedMeshRendererPtr& meshRenderer,
		const video::TextureAtlasRendererPtr& textureAtlasRenderer);
};
