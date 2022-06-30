/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "image/Image.h"
#include "io/Filesystem.h"
#include "math/Axis.h"
#include "video/Texture.h"
#include "video/TexturePool.h"

namespace voxedit {

class ToolsPanel {
private:
	bool mirrorAxisRadioButton(const char *title, math::Axis type);
	void loadTextures(const core::String &dir);
	video::TexturePool _texturePool;
	io::FilesystemPtr _filesystem;
public:
	ToolsPanel(const io::FilesystemPtr &filesystem);
	void update(const char *title, command::CommandExecutionListener &listener);
};

} // namespace voxedit
