/**
 * @file
 */

#pragma once

#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "testcore/TestApp.h"
#include "voxel/MeshState.h"
#include "voxelrender/RawVolumeRenderer.h"
#include "voxelrender/RenderContext.h"
#include <glm/mat4x4.hpp>

class TestVolumeRenderer : public TestApp {
private:
	using Super = TestApp;
	voxelrender::RawVolumeRenderer _renderer;
	voxel::MeshStatePtr _meshState;
	voxelrender::RenderContext _renderContext;
	palette::Palette _palette;
	scenegraph::SceneGraph _sceneGraph;
	bool _ownsVolumes = true;

	void doRender() override;
	bool createMaterialScene();
	bool loadScene(const core::String &filename);
	bool addVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette, const glm::mat4 &worldMatrix);
	void configureRendererFeatures();
	void flushMeshes();
	void presentFrameBuffer();

public:
	TestVolumeRenderer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	void onWindowResize(void *windowHandle, int windowWidth, int windowHeight) override;
};
