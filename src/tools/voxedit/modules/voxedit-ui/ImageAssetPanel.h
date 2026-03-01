/**
 * @file
 */

#pragma once

#include "core/collection/ConcurrentQueue.h"
#include "io/Filesystem.h"
#include "ui/Panel.h"
#include "video/TexturePool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

class ImageAssetPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	video::TexturePoolPtr _texturePool;
	io::FilesystemPtr _filesystem;
	SceneManagerPtr _sceneMgr;
	core::ConcurrentQueue<image::ImagePtr> _images;

public:
	ImageAssetPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr, const video::TexturePoolPtr &texturePool,
					const io::FilesystemPtr &filesystem);
	bool init();
	void update(const char *id);
	void shutdown();
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
