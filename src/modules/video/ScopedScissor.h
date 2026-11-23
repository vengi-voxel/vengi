#include "math/Vec2i.h"
/**
 * @file
 */
#pragma once


namespace video {

/**
 * @brief Restore the previous scissor after leaving the scope of the object
 * @ingroup Video
 */
class ScopedScissor {
private:
	int _x;
	int _y;
	int _w;
	int _h;
	bool _oldState;
public:
	ScopedScissor(int x, int y, int w, int h);
	ScopedScissor(const math::Vec2i& pos, const math::Vec2i& size);
	ScopedScissor(const math::Vec2i& pos, int w, int h);

	~ScopedScissor();
};

}
