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
	bool _valid;
public:
	static const MoveVector Invalid;

	MoveVector(const glm::vec3& vec3, float rotation, bool valid = true);

	float getOrientation(float duration) const ;

	inline bool isValid() const {
		return _valid;
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
