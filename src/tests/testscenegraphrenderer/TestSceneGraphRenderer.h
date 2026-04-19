/**
 * @file
 */

#pragma once

#include "scenegraph/SceneGraph.h"
#include "testcore/TestApp.h"
#include "voxel/MeshState.h"
#include "voxelrender/RenderContext.h"
#include "voxelrender/SceneGraphRenderer.h"

class TestSceneGraphRenderer : public TestApp {
private:
	using Super = TestApp;
	voxelrender::SceneGraphRenderer _renderer;
	voxel::MeshStatePtr _meshState;
	voxelrender::RenderContext _renderContext;
	scenegraph::SceneGraph _sceneGraph;

	void doRender() override;

public:
	TestSceneGraphRenderer(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);

	app::AppState onInit() override;
	app::AppState onCleanup() override;
};
