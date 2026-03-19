/**
 * @file
 */

#pragma once

#include <glm/common.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace math {

struct BezierSegment {
	glm::ivec3 start{0};
	glm::ivec3 end{0};
	glm::ivec3 control{0};
};

template<typename TYPE = float, glm::qualifier P = glm::defaultp>
class Bezier {
public:
	using Vec = glm::vec<3, TYPE, P>;
	using EvalVec = glm::vec<3, float, P>;

private:
	Vec _start;
	Vec _end;
	Vec _control;
public:
	Bezier(const Vec& start, const Vec& end, const Vec& control) :
			_start(start), _end(end), _control(control) {
	}

	inline EvalVec evaluate(float p) const {
		const float i = 1.0f - p;
		const float p0 = i * i;
		const float p1 = 2.0f * p * i;
		const float b2 = p * p;

		const float x = _start.x * p0 + _control.x * p1 + _end.x * b2;
		const float y = _start.y * p0 + _control.y * p1 + _end.y * b2;
		const float z = _start.z * p0 + _control.z * p1 + _end.z * b2;
		return EvalVec(x, y, z);
	}

	inline Vec getPoint(float p) const {
		const EvalVec point = evaluate(p);
		return Vec((TYPE)(point.x + 0.5f), (TYPE)(point.y + 0.5f), (TYPE)(point.z + 0.5f));
	}

	inline float estimateLength() const {
		const EvalVec start(_start);
		const EvalVec end(_end);
		const EvalVec control(_control);
		return glm::length(control - start) + glm::length(end - control);
	}

	inline int estimateSteps() const {
		int steps = glm::max(1, (int)glm::ceil(estimateLength()));
		if ((steps & 1) != 0) {
			++steps;
		}
		return steps;
	}

	template<typename FUNC>
	inline void visitPoints(int steps, FUNC&& func) const {
		const int clampedSteps = glm::max(1, steps <= 0 ? estimateSteps() : steps);
		const float stepSize = 1.0f / (float)clampedSteps;
		for (int i = 0; i <= clampedSteps; ++i) {
			const float t = stepSize * (float)i;
			func(getPoint(t));
		}
	}

	template<typename FUNC>
	inline void visitSegments(int steps, FUNC&& func) const {
		const int clampedSteps = glm::max(1, steps <= 0 ? estimateSteps() : steps);
		const float stepSize = 1.0f / (float)clampedSteps;
		Vec lastPos = getPoint(0.0f);
		for (int i = 1; i <= clampedSteps; ++i) {
			const float t = stepSize * (float)i;
			const Vec pos = getPoint(t);
			func(lastPos, pos);
			lastPos = pos;
		}
	}
};

}
