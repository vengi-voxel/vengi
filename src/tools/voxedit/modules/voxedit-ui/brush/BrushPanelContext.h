/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/Var.h"

namespace ui {
class IMGUIApp;
}

namespace video {
class TexturePool;
typedef core::SharedPtr<TexturePool> TexturePoolPtr;
} // namespace video

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

/**
 * @brief Shared dependencies passed to brush panel sections while building settings UI.
 */
struct BrushPanelContext {
	ui::IMGUIApp *app = nullptr;
	SceneManagerPtr sceneMgr;
	video::TexturePoolPtr texturePool;
	core::VarPtr renderNormals;
	core::VarPtr viewMode;
};

} // namespace voxedit
