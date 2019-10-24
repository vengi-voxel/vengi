/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <vector>
#include "RenderShaders.h"
#include "video/FrameBuffer.h"
#include "video/Buffer.h"
#include "video/ScopedPolygonMode.h"

namespace video {
class Camera;
}

namespace render {

struct ShadowParameters {
	/**
	 * @brief This value defines the amount of shadow cascades that are used
	 * @note This value must not be modified after the shadow instance was initialized with it.
	 */
	int maxDepthBuffers = -1;

	float shadowBiasSlope = 2.0f;
	float shadowBias = 0.09f;
	float sliceWeight = 0.05f;
};

/**
 * @brief Helper class for calculating the cascaded shadow map data
 */
class Shadow {
private:
	glm::vec3 _sunDirection;
	glm::mat4 _lightView;
	std::vector<glm::mat4> _cascades;
	std::vector<float> _distances;
	video::Buffer _shadowMapDebugBuffer;
	video::FrameBuffer _depthBuffer;
	shader::ShadowmapShader& _shadowMapShader;
	shader::ShadowmapRenderShader& _shadowMapRenderShader;
	shader::ShadowmapInstancedShader& _shadowMapInstancedShader;
	ShadowParameters _parameters;
	float _shadowRangeZ = 0.0f;

public:
	Shadow();
	~Shadow();
	/**
	 * @param[in] maxDepthBuffers the amount of cascades
	 */
	bool init(const ShadowParameters& parameters);
	void shutdown();

	typedef std::function<bool(int, shader::ShadowmapShader&)> funcRender;
	typedef std::function<bool(int, shader::ShadowmapInstancedShader&)> funcRenderInstance;

	void update(const video::Camera& camera, bool active);

	bool bind(video::TextureUnit unit);

	void renderShadowMap(const video::Camera& camera);

	void render(funcRender renderCallback, funcRenderInstance renderInstancedCallback);

	void setPosition(const glm::vec3& eye, const glm::vec3& center = glm::zero<glm::vec3>(), const glm::vec3& up = glm::up);
	void setLightViewMatrix(const glm::mat4& lightView);

	ShadowParameters& parameters();

	const std::vector<glm::mat4>& cascades() const;
	const std::vector<float>& distances() const;
	const glm::vec3& sunDirection() const;
	const glm::ivec2& dimension() const;
	glm::vec3 sunPosition() const;
};

inline const std::vector<glm::mat4>& Shadow::cascades() const {
	return _cascades;
}

inline const std::vector<float>& Shadow::distances() const {
	return _distances;
}

inline const glm::vec3& Shadow::sunDirection() const {
	return _sunDirection;
}

inline ShadowParameters& Shadow::parameters() {
	return _parameters;
}
}
