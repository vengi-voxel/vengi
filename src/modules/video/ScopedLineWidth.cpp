#include "ScopedLineWidth.h"

namespace video {

glm::vec2 ScopedLineWidth::_smoothedLineWidth = glm::vec2(-1.0f);
glm::vec2 ScopedLineWidth::_aliasedLineWidth = glm::vec2(-1.0f);

ScopedLineWidth::ScopedLineWidth(float width, bool antialiasing) :
		_antialiasing(antialiasing) {
	if (_smoothedLineWidth.x < 0.0f) {
		GLdouble buf[2];
		glGetDoublev(GL_SMOOTH_LINE_WIDTH_RANGE, buf);
		_smoothedLineWidth.x = (float)buf[0];
		_smoothedLineWidth.y = (float)buf[1];
		glGetDoublev(GL_ALIASED_LINE_WIDTH_RANGE, buf);
		_aliasedLineWidth.x = (float)buf[0];
		_aliasedLineWidth.y = (float)buf[1];
		// TODO GL_SMOOTH_LINE_WIDTH_GRANULARITY
	}
	glGetFloatv(GL_LINE_WIDTH, &_restoreValue);
	if (_antialiasing) {
		_antialiasingWasEnabled = glIsEnabled(GL_LINE_SMOOTH);
		if (!_antialiasingWasEnabled) {
			video::enable(video::State::LineSmooth);
		}
		glLineWidth(glm::clamp(width, _smoothedLineWidth.x, _smoothedLineWidth.y));
	} else {
		glLineWidth(glm::clamp(width, _aliasedLineWidth.x, _aliasedLineWidth.y));
	}
	video::checkError();
}

ScopedLineWidth::~ScopedLineWidth() {
	glLineWidth(_restoreValue);
	if (_antialiasing && !_antialiasingWasEnabled) {
		video::disable(video::State::LineSmooth);
	}
	video::checkError();
}

}
