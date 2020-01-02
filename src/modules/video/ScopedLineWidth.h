/**
 * @file
 * https://www.opengl.org/sdk/docs/man/docbook4/xhtml/glLineWidth.xml
 */

#pragma once

namespace video {

/**
 * @brief Not all line widths are supported on all platforms. This class helps you to set
 * a supported width and also restore the previous state after the scope was left.
 * @ingroup Video
 */
class ScopedLineWidth {
private:
	float _oldWidth = 1.0f;
	bool _oldAntialiasing = false;
	bool _antialiasing;
public:
	ScopedLineWidth(float width, bool smooth = false);
	~ScopedLineWidth();
};

}
