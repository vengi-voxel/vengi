/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "io/Filesystem.h"
#include "math/Axis.h"
#include "video/Texture.h"
#include "video/TexturePool.h"

namespace voxedit {

class AssetPanel {
private:
	void loadTextures(const core::String &dir);
	void loadModels(const core::String &dir);
	video::TexturePool _texturePool;
	core::DynamicArray<core::String> _models;
	io::FilesystemPtr _filesystem;
	int _currentSelectedModel = 0;
public:
	AssetPanel(const io::FilesystemPtr &filesystem);
	void update(const char *title, bool sceneMode, command::CommandExecutionListener &listener);
};

} // namespace voxedit
