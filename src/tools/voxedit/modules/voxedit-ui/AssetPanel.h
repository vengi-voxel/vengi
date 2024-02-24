/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "command/CommandHandler.h"
#include "core/collection/DynamicArray.h"
#include "video/TexturePool.h"

namespace voxedit {

class AssetPanel : public ui::Panel {
private:
	using Super = ui::Panel;

	void loadTextures(const core::String &dir);
	void loadModels(const core::String &dir);
	video::TexturePool _texturePool;
	core::DynamicArray<core::String> _models;
	int _currentSelectedModel = 0;
public:
	AssetPanel(ui::IMGUIApp *app);
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
};

} // namespace voxedit
