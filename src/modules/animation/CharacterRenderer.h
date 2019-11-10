/**
 * @file
 */

#pragma once

#include "Character.h"
#include "core/IComponent.h"
#include "AnimationShaders.h"
#include "video/Buffer.h"
#include "video/Camera.h"
#include "render/Shadow.h"
#include "core/Color.h"

namespace animation {

/**
 * @brief Uses the shaders from the animation module to render characters.
 */
class CharacterRenderer : public core::IComponent {
private:
	shader::CharacterShader _shader;
	shader::CharacterData _shaderData;
	render::Shadow _shadow;
	video::Buffer _vbo;

	float _fogRange = 300.0f;
	glm::vec4 _clearColor = core::Color::LightBlue;
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);

	int32_t _vertices = -1;
	int32_t _indices = -1;
public:
	bool init() override;
	void shutdown() override;

	void setClearColor(const glm::vec4& c) { _clearColor = c; }
	void setDiffuseColor(const glm::vec3& c) { _diffuseColor = c; }
	void setAmbientColor(const glm::vec3& c) { _ambientColor = c; }

	/**
	 * @brief Render the given character instance
	 * @note Make sure to update the character before calling this method in order
	 * to update the bones.
	 */
	void render(const Character& character, const video::Camera& camera);
};

}
