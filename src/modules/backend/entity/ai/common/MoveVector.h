/**
 * @file
 */
#pragma once

#include <glm/vec3.hpp>

namespace backend {

class MoveVector {
protected:
	const glm::vec3 _vec3;
	const float _rotation;
public:
	MoveVector(const glm::vec3& vec3, float rotation) :
			_vec3(vec3), _rotation(rotation) {
	}

	MoveVector(const glm::vec3& vec3, double rotation) :
			_vec3(vec3), _rotation(static_cast<float>(rotation)) {
	}

	inline float getOrientation(float duration) const {
		const float pi2 = glm::two_pi<float>();
		const float rotation = _rotation + pi2;
		return fmodf(rotation * duration, pi2);
	}

	inline const glm::vec3& getVector() const {
		return _vec3;
	}

	inline glm::vec3 getVector() {
		return _vec3;
	}

	inline float getRotation() const {
		return _rotation;
	}

	inline operator glm::vec3() const {
		return _vec3;
	}

	inline operator const glm::vec3&() const {
		return _vec3;
	}

	inline operator float() const {
		return _rotation;
	}
};

}
