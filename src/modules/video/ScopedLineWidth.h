/**
 * @file
 * https://www.opengl.org/sdk/docs/man/docbook4/xhtml/glLineWidth.xml
 */

#pragma once

#include "GLFunc.h"
#include "core/GLM.h"

namespace video {

/**
 * @brief Not all line widths are supported on all platforms. This class helps you to set
 * a supported width and also restore the previous state after the scope was left.
 */
class ScopedLineWidth {
private:
	static glm::vec2 _smoothedLineWidth;
	static glm::vec2 _aliasedLineWidth;

	float _restoreValue = 1.0f;
	bool _antialiasingWasEnabled = false;
	bool _antialiasing;
public:
	ScopedLineWidth(float width, bool smooth = false);
	~ScopedLineWidth();
};

}
