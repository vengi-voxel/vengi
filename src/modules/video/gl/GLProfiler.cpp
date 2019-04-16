/**
 * @file
 */

#include "video/Renderer.h"
#include "flextGL.h"
#include <algorithm>

namespace video {

bool ProfilerGPU::init() {
	glGenQueries(1, &_id);
	return _id != InvalidId;
}

void ProfilerGPU::shutdown() {
	if (_id == InvalidId) {
		return;
	}
	glDeleteQueries(1, &_id);
	_id = InvalidId;
}

void ProfilerGPU::enter() {
	if (_id == InvalidId) {
		return;
	}
	core_assert(_state == 0 || _state == 2);

	if (_state == 0) {
		glBeginQuery(GL_TIME_ELAPSED, _id);
		_state = 1;
	}
}

void ProfilerGPU::leave() {
	if (_id == InvalidId) {
		return;
	}
	core_assert(_state == 1 || _state == 2);

	if (_state == 1) {
		glEndQuery(GL_TIME_ELAPSED);
		_state = 2;
	} else if (_state == 2) {
		GLint availableResults = 0;
		glGetQueryObjectiv(_id, GL_QUERY_RESULT_AVAILABLE, &availableResults);
		if (availableResults > 0) {
			_state = 0;
			GLuint64 time = 0;
			glGetQueryObjectui64v(_id, GL_QUERY_RESULT, &time);
			const double timed = double(time);
			_samples[_sampleCount & (_maxSampleCount - 1)] = timed;
			++_sampleCount;
			_max = std::max(_max, timed);
			_min = std::min(_min, timed);
			_avg = _avg * 0.5 + timed / 1e9 * 0.5;
		}
	}
}

}
