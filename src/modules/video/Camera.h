#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include "io/IEventObserver.h"
#include "core/Var.h"
#include <math.h>
#include <ctime>

namespace video {

class Camera {
private:
	glm::vec3 _pos;
	glm::mat4 _viewMatrix;
	int _width;
	int _height;
	float _pitch;
	float _yaw;
	glm::vec3 _direction;
	core::VarPtr _maxpitch;

public:
	Camera();
	~Camera();
	void init(int width, int height);

	void onMotion(int32_t x, int32_t y, int32_t relX, int32_t relY, float rotationSpeed = 0.01f);
	void onMovement(int32_t forward, int32_t sideward);

	void updatePosition(long dt, bool left, bool right, bool forward, bool backward, float speed = 0.01f);

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	void updateDirection();

	inline void updateViewMatrix() {
		_viewMatrix = glm::lookAt(_pos, _pos + glm::normalize(_direction), glm::vec3(0.0, 1.0, 0.0));
	}

	inline void update() {
		updateDirection();
		updateViewMatrix();
	}

	inline const glm::vec3 getPosition() const {
		return _pos;
	}

	/**
	 * @brief Return the horizontal angle of the camera view direction
	 */
	inline float yaw() const {
		return _yaw;
	}

	/**
	 * @brief Return the vertical angle of the camera view direction
	 */
	inline float pitch() const {
		return _pitch;
	}

	inline void setAngles(float pitch, float yaw) {
		_pitch = pitch;
		_yaw = yaw;
	}

	inline void setPosition(const glm::vec3& pos) {
		_pos = pos;
	}

	inline const glm::mat4& getViewMatrix() const {
		return _viewMatrix;
	}
};

}
