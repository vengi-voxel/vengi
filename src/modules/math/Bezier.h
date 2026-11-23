#include "math/VecOf.h"
/**
 * @file
 */

#pragma once


namespace math {

struct BezierSegment {
	math::Vec3i start{0};
	math::Vec3i end{0};
	math::Vec3i control{0};
};

template<typename TYPE = float>
class Bezier {
public:
	using Vec = typename Vec3Of<TYPE>::Type;
	using EvalVec = math::Vec3f;

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
		return math::length(control - start) + math::length(end - control);
	}

	inline int estimateSteps() const {
		int steps = math::max(1, (int)math::ceil(estimateLength()));
		if ((steps & 1) != 0) {
			++steps;
		}
		return steps;
	}

	template<typename FUNC>
	inline void visitPoints(int steps, FUNC&& func) const {
		const int clampedSteps = math::max(1, steps <= 0 ? estimateSteps() : steps);
		const float stepSize = 1.0f / (float)clampedSteps;
		for (int i = 0; i <= clampedSteps; ++i) {
			const float t = stepSize * (float)i;
			func(getPoint(t));
		}
	}

	template<typename FUNC>
	inline void visitSegments(int steps, FUNC&& func) const {
		const int clampedSteps = math::max(1, steps <= 0 ? estimateSteps() : steps);
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
