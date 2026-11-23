/**
 * @file
 */

#pragma once

#include "VoxelShaderConstants.h"
#include "video/FrameBuffer.h"
#include "video/Buffer.h"
#include "core/Function.h"
#include "core/collection/Array.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"
#include "math/Vec2i.h"

namespace video {
class Camera;
}

namespace voxelrender {

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

	/** Used to slice the camera frustum. 0 = linear, 1 = logarithmic (tighter near cascades). */
	float sliceWeight = 0.75f;
};

/**
 * @brief Helper class for calculating the cascaded shadow map data
 * @sa ShadowParameters
 */
class Shadow {
public:
	using Cascades = core::Array<math::Mat4f, shader::VoxelShaderConstants::getMaxDepthBuffers()>;
	using Distances = core::Array<float, shader::VoxelShaderConstants::getMaxDepthBuffers()>;
private:
	math::Vec3f _sunDirection{};
	math::Mat4f _lightView = math::Mat4f::identity();
	Cascades _cascades;
	Distances _distances;
	video::FrameBuffer _depthBuffer;
	ShadowParameters _parameters;

public:
	Shadow() {
		_cascades.fill(math::Mat4f::identity());
	}
	~Shadow();

	bool init(const ShadowParameters& parameters);
	void shutdown();

	typedef core::Function<bool(int, const math::Mat4f& viewProjection)> funcRender;

	void update(const video::Camera& camera, bool active, const math::Vec3f& sceneMins = math::Vec3f(0.0f),
				const math::Vec3f& sceneMaxs = math::Vec3f(0.0f));

	bool bind(video::TextureUnit unit);

	void render(const funcRender& renderCallback, bool clearDepthBuffer = true);

	void setPosition(const math::Vec3f& eye, const math::Vec3f& center, const math::Vec3f& up);
	void setLightViewMatrix(const math::Mat4f& lightView);

	video::FrameBuffer& depthBuffer();

	const Cascades& cascades() const;
	const Distances& distances() const;
	const math::Vec3f& sunDirection() const;
	const math::Vec2i& dimension() const;
	math::Vec3f sunPosition() const;
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

inline const math::Vec3f& Shadow::sunDirection() const {
	return _sunDirection;
}

}
