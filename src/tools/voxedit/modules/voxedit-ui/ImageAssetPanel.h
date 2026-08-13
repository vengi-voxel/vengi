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

	// Longer side of asset panel preview thumbnails (display size is ~50px * dpi)
	static constexpr int ThumbnailSize = 128;

	video::TexturePoolPtr _texturePool;
	io::FilesystemPtr _filesystem;
	SceneManagerPtr _sceneMgr;
	core::ConcurrentQueue<image::ImagePtr> _images;
	// Full-resolution image kept alive for the current drag-drop payload
	image::ImagePtr _dragImage;

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
