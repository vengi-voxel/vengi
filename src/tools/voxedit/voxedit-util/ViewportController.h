/**
 * @file
 */

#pragma once

#include "voxel/polyvox/RawVolume.h"
#include "core/Var.h"
#include "video/Camera.h"
#include <SDL.h>

namespace voxedit {

/**
 * @brief This is the viewport controller
 *
 * @sa Viewport
 */
class ViewportController {
public:
	enum class SceneCameraMode : uint8_t {
		Free, Top, Left, Front
	};

	enum class ShaderType {
		None,
		Edge,

		Max
	};

private:
	float _angle = 0.0f;
	SceneCameraMode _camMode = SceneCameraMode::Free;
	core::VarPtr _rotationSpeed;
	video::Camera _camera;
	ShaderType _shaderType = ShaderType::None;

public:
	bool _mouseDown = false;
	int _mouseX = 0;
	int _mouseY = 0;

	void init(ViewportController::SceneCameraMode mode);
	void resetCamera(const voxel::Region& region);

	void setShaderType(ShaderType type);
	ShaderType shaderType() const;

	void onResize(const glm::ivec2& frameBufferSize, const glm::ivec2& windowSize);

	void update(long deltaFrame);

	bool move(bool rotate, int x, int y);

	video::Camera& camera();

	float angle() const;
	void setAngle(float angle);
};

inline video::Camera& ViewportController::camera() {
	return _camera;
}

inline float ViewportController::angle() const {
	return _angle;
}

inline void ViewportController::setAngle(float angle) {
	_angle = angle;
}

inline ViewportController::ShaderType ViewportController::shaderType() const {
	return _shaderType;
}

inline void ViewportController::setShaderType(ShaderType type) {
	_shaderType = type;
}

}
