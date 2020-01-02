/**
 * @file
 */
#pragma once

namespace video {

/**
 * @brief Restore the previous viewport after leaving the scope of the object
 */
class ScopedViewPort {
private:
	int _x;
	int _y;
	int _w;
	int _h;
public:
	/**
	 * @note Keep in mind that opengl y starts from below - these are no screen coordinates, but opengl coordinates.
	 */
	ScopedViewPort(int x, int y, int w, int h);
	~ScopedViewPort();
};

}
