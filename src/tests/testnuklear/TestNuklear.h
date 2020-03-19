/**
 * @file
 */

#pragma once

#include "ui/nuklear/NuklearApp.h"

class TestNuklear: public ui::nuklear::NuklearApp {
private:
	using Super = ui::nuklear::NuklearApp;
public:
	TestNuklear(const metric::MetricPtr& metric,
		const io::FilesystemPtr& filesystem,
		const core::EventBusPtr& eventBus,
		const core::TimeProviderPtr& timeProvider,
		const video::TexturePoolPtr& texturePool,
		const voxelrender::CachedMeshRendererPtr& meshRenderer,
		const video::TextureAtlasRendererPtr& textureAtlasRenderer);

	void initUIFonts() override;

	core::AppState onInit() override;
	bool onRenderUI() override;
};
