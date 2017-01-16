#include "ScopedLineWidth.h"

namespace video {

ScopedLineWidth::ScopedLineWidth(float width, bool antialiasing) :
		_antialiasing(antialiasing) {
	if (_antialiasing) {
		_oldAntialiasing = video::enable(video::State::LineSmooth);
	}
	_oldWidth = video::lineWidth(width);
}

ScopedLineWidth::~ScopedLineWidth() {
	video::lineWidth(_oldAntialiasing);
	if (_antialiasing && !_oldAntialiasing) {
		video::disable(video::State::LineSmooth);
	}
}

}
