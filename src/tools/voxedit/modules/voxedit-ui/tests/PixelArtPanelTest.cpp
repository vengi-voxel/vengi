/**
 * @file
 */

#include "../PixelArtPanel.h"
#include "TestUtil.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "image/Image.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxedit {

void PixelArtPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	IM_REGISTER_TEST(engine, testCategory(), "toggle from view menu")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Pixel art", cfg::VoxEditShowPixelArt));
		IM_CHECK(focusWindow(ctx, id));
	};

	IM_REGISTER_TEST(engine, testCategory(), "preview active model")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(prepareUITest(ctx));
		IM_CHECK(ensurePanelVisible(ctx, "View/Pixel art", cfg::VoxEditShowPixelArt));
		_view = 0;
		core::getVar(cfg::ImageIsometric)->setVal(true);
		_hash = 0;
		IM_CHECK(_sceneMgr->newScene(true, "pixelart", voxel::Region(0, 7)));
		ctx->Yield();
		IM_CHECK(focusWindow(ctx, id));

		scenegraph::SceneGraphNode *model = _sceneMgr->sceneGraphModelNodeByUUID(_sceneMgr->activeNodeUUID());
		IM_CHECK(model != nullptr);
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(0, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 1)));
		IM_CHECK(setVoxel(_sceneMgr, model, glm::ivec3(2, 0, 0), voxel::createVoxel(voxel::VoxelType::Generic, 2)));
		ctx->Yield();

		IM_CHECK(_image && _image->isLoaded());
		IM_CHECK_GT(_image->width(), 0);
		IM_CHECK_GT(_image->height(), 0);
		const image::ImagePtr front = _image;

		_view = 1;
		_hash = 0;
		ctx->Yield();
		IM_CHECK(_image && _image->isLoaded());
		IM_CHECK(_image.get() != front.get());
		IM_CHECK_EQ(_view, 1);
	};
}

} // namespace voxedit
