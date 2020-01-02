/**
 * @file
 */
#pragma once

#include <glm/vec2.hpp>

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
	ScopedScissor(const glm::ivec2& pos, const glm::ivec2& size);
	ScopedScissor(const glm::ivec2& pos, int w, int h);

	~ScopedScissor();
};

}
