/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "image/Image.h"
#include "ui/Panel.h"
#include "video/Texture.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

/**
 * @brief Pixel-art preview of the active model (isometric or orthographic 2D).
 */
class PixelArtPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;
	int _view = 0;
	// Integer framebuffer pixels per source pixel. 0 until the first layout.
	// Scroll-wheel steps change this by 1 so every texel stays a sharp square.
	int _zoom = 0;
	bool _zoomUser = false;
	float _zoomWheel = 0.0f;
	uint64_t _configHash = 0;
	uint64_t _hash = 0;
	int _node = InvalidNodeId;
	image::ImagePtr _image;
	video::TexturePtr _texture;

	image::ImagePtr pixelArtImage();
	core::String suggestedSaveName() const;
	void saveImage();

public:
	PixelArtPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "pixelart"), _sceneMgr(sceneMgr) {
	}
	~PixelArtPanel();

	void update(const char *id);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
