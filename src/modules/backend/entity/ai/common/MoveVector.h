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
	MoveVector(const glm::vec3& vec3, float rotation);

	MoveVector(const glm::vec3& vec3, double rotation);

	float getOrientation(float duration) const ;

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
