/**
 * @file
 */

#include "ScopedViewPort.h"
#include "Renderer.h"

namespace video {

ScopedViewPort::ScopedViewPort(int x, int y, int w, int h) {
	video::getViewport(_x, _y, _w, _h);
	video::viewport(x, y, w, h);
}

ScopedViewPort::~ScopedViewPort() {
	video::viewport(_x, _y, _w, _h);
}

}
