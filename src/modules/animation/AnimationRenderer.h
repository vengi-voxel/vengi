/**
 * @file
 */

#pragma once

#include "AnimationEntity.h"
#include "core/IComponent.h"
#include "AnimationShaders.h"
#include "video/Buffer.h"
#include "render/Shadow.h"
#include "core/Color.h"

namespace video {
class Camera;
}

namespace animation {

/**
 * @brief Uses the shaders from the animation module to render entities.
 * @ingroup Animation
 */
class AnimationRenderer : public core::IComponent {
private:
	shader::SkeletonShader& _shader;
	shader::SkeletonshadowmapShader& _shadowMapShader;
	shader::SkeletonData _shaderData;
	render::Shadow _shadow;
	video::Buffer _vbo;

	double _seconds = 0.0;
	float _fogRange = 300.0f;
	glm::vec4 _clearColor = core::Color::LightBlue;
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);
	glm::vec3 _nightColor = glm::vec3(0.001, 0.001, 0.2);

	int32_t _vertices = -1;
	int32_t _indices = -1;
public:
	AnimationRenderer();
	bool init() override;
	void shutdown() override;

	void setClearColor(const glm::vec4& c) { _clearColor = c; }
	void setDiffuseColor(const glm::vec3& c) { _diffuseColor = c; }
	void setAmbientColor(const glm::vec3& c) { _ambientColor = c; }

	void setSeconds(double seconds);

	/**
	 * @brief Render the given character instance
	 * @note Make sure to update the character before calling this method in order
	 * to update the bones.
	 */
	void render(const AnimationEntity& character, const video::Camera& camera);
};

inline void AnimationRenderer::setSeconds(double seconds) {
	_seconds = seconds;
}

}
