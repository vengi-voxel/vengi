/**
 * @file
 */

#pragma once

#include "ConstantsShaderConstants.h"
#include "video/FrameBuffer.h"
#include "video/Buffer.h"
#include <functional>
#include "core/collection/Array.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

namespace video {
class Camera;
}

namespace render {

/**
 * @brief Paramerters to set up the @c Shadow instance
 * @sa Shadow
 */
struct ShadowParameters {
	/**
	 * @brief This value defines the amount of shadow cascades that are used
	 * @note This value must not be modified after the shadow instance was initialized with it.
	 */
	int maxDepthBuffers = -1;

	/** set the scale used to calculate depth values */
	float shadowBiasSlope = 2.0f;
	/** influences the units used to calculate depth values */
	float shadowBias = 0.09f;
	/** Used to slice the camera frustum */
	float sliceWeight = -0.3f;
};

/**
 * @brief Helper class for calculating the cascaded shadow map data
 * @sa ShadowParameters
 */
class Shadow {
public:
	using Cascades = core::Array<glm::mat4, shader::ConstantsShaderConstants::getMaxDepthBuffers()>;
	using Distances = core::Array<float, shader::ConstantsShaderConstants::getMaxDepthBuffers()>;
private:
	glm::vec3 _sunDirection;
	glm::mat4 _lightView;
	Cascades _cascades;
	Distances _distances;
	video::FrameBuffer _depthBuffer;
	ShadowParameters _parameters;
	float _shadowRangeZ = 0.0f;

	glm::vec4 splitFrustumSphereBoundingBox(const video::Camera& camera, float near, float far) const;

public:
	~Shadow();

	bool init(const ShadowParameters& parameters);
	void shutdown();

	typedef std::function<bool(int, const glm::mat4& viewProjection)> funcRender;

	void update(const video::Camera& camera, bool active);

	bool bind(video::TextureUnit unit);

	void render(const funcRender& renderCallback, bool clearDepthBuffer = true);

	void setPosition(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up);
	void setLightViewMatrix(const glm::mat4& lightView);

	video::FrameBuffer& depthBuffer();

	ShadowParameters& parameters();

	const Cascades& cascades() const;
	const Distances& distances() const;
	const glm::vec3& sunDirection() const;
	const glm::ivec2& dimension() const;
	glm::vec3 sunPosition() const;
};

inline video::FrameBuffer& Shadow::depthBuffer() {
	return _depthBuffer;
}

inline const Shadow::Cascades& Shadow::cascades() const {
	return _cascades;
}

inline const Shadow::Distances& Shadow::distances() const {
	return _distances;
}

inline const glm::vec3& Shadow::sunDirection() const {
	return _sunDirection;
}

inline ShadowParameters& Shadow::parameters() {
	return _parameters;
}
}
