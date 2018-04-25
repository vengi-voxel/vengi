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
#include "video/VertexBuffer.h"
#include "video/ScopedPolygonMode.h"

namespace video {
class Camera;
}

namespace render {

/**
 * @brief Helper class for calculating the cascaded shadow map data
 */
class Shadow {
private:
	glm::vec3 _sunDirection;
	glm::mat4 _lightView;
	std::vector<glm::mat4> _cascades;
	std::vector<float> _distances;
	video::VertexBuffer _shadowMapDebugBuffer;
	video::FrameBuffer _depthBuffer;
	shader::ShadowmapShader& _shadowMapShader;
	shader::ShadowmapRenderShader& _shadowMapRenderShader;
	shader::ShadowmapInstancedShader& _shadowMapInstancedShader;
	float _shadowBiasSlope = 2.0f;
	float _shadowBias = 0.09f;
	float _shadowRangeZ = 0.0f;
	int _maxDepthBuffers = 0;

public:
	Shadow();
	bool init(int maxDepthBuffers);
	void shutdown();

	/**
	 * @param[in] maxDepthBuffers the amount of cascades
	 */
	void calculateShadowData(const video::Camera& camera, bool active, float sliceWeight = 1.0f);

	bool bind(video::TextureUnit unit);

	void renderShadowMap(const video::Camera& camera);

	template<typename FUNC, typename FUNCINSTANCED>
	void render(FUNC&& renderCallback, FUNCINSTANCED&& renderInstancedCallback) {
		const bool oldBlend = video::disable(video::State::Blend);
		// put shadow acne into the dark
		const bool cullFaceChanged = video::cullFace(video::Face::Front);
		const glm::vec2 offset(_shadowBiasSlope, (_shadowBias / _shadowRangeZ) * (1 << 24));
		const video::ScopedPolygonMode scopedPolygonMode(video::PolygonMode::Solid, offset);

		_depthBuffer.bind(false);
		for (int i = 0; i < _maxDepthBuffers; ++i) {
			_depthBuffer.bindTextureAttachment(video::FrameBufferAttachment::Depth, i, true);
			{
				video::ScopedShader scoped(_shadowMapShader);
				_shadowMapShader.setLightviewprojection(_cascades[i]);
				if (!renderCallback(i, _shadowMapShader)) {
					break;
				}
			}
			{
				video::ScopedShader scoped(_shadowMapInstancedShader);
				_shadowMapInstancedShader.setLightviewprojection(_cascades[i]);
				if (!renderInstancedCallback(i, _shadowMapInstancedShader)) {
					break;
				}
			}
		}
		_depthBuffer.unbind();
		if (cullFaceChanged) {
			video::cullFace(video::Face::Back);
		}
		if (oldBlend) {
			video::enable(video::State::Blend);
		}
	}

	void setShadowRangeZ(float shadowRangeZ);

	const std::vector<glm::mat4>& cascades() const;
	const std::vector<float>& distances() const;
	const glm::vec3& sunDirection() const;
	const glm::ivec2& dimension() const;
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

inline void Shadow::setShadowRangeZ(float shadowRangeZ) {
	_shadowRangeZ = shadowRangeZ;
}

}
