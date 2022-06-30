/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "image/Image.h"
#include "math/Axis.h"
#include "video/Texture.h"

namespace voxedit {

class ToolsPanel {
private:
	bool mirrorAxisRadioButton(const char *title, math::Axis type);
	image::ImagePtr _image;
	video::TexturePtr _texture;

public:
	void update(const char *title, command::CommandExecutionListener &listener);
};

} // namespace voxedit
