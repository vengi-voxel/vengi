/**
 * @file
 * https://www.opengl.org/sdk/docs/man/docbook4/xhtml/glLineWidth.xml
 */

#pragma once

#include "Types.h"

namespace video {

class ScopedBlendMode {
private:
	bool _restore;
	GLboolean _blendEnabled;
	GLint _oldSrc;
	GLint _oldDest;
public:
	ScopedBlendMode(video::BlendMode src, video::BlendMode dest, bool restore = true);
	~ScopedBlendMode();
};

}
