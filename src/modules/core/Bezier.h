#pragma once

#include <glm/vec3.hpp>

namespace core {

template<typename TYPE = float, glm::precision P = glm::defaultp>
class Bezier {
private:
	glm::tvec3<TYPE, P> _start;
	glm::tvec3<TYPE, P> _end;
	glm::tvec3<TYPE, P> _control;
public:
	Bezier(const glm::tvec3<TYPE, P>& start, const glm::tvec3<TYPE, P>& end, const glm::tvec3<TYPE, P>& control) :
			_start(start), _end(end), _control(control) {
	}

	const glm::tvec3<TYPE> getPoint(float p) const {
		const float i = 1.0f - p;
		const float p0 = i * i;
		const float p1 = 2.0f * p * i;
		const float b2 = p * p;

		const float x = _start.x * p0 + _control.x * p1 + _end.x * b2;
		const float y = _start.y * p0 + _control.y * p1 + _end.y * b2;
		const float z = _start.z * p0 + _control.z * p1 + _end.z * b2;

		return glm::tvec3<TYPE, P>((TYPE)(x + 0.5f), (TYPE)(y + 0.5f), (TYPE)(z + 0.5f));
	}
};

}
