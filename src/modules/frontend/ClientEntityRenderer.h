/**
 * @file
 */

#pragma once

#include "AnimationShaders.h"
#include "core/IComponent.h"
#include "video/FrameBuffer.h"
#include "core/Var.h"

namespace core {
template<class T>
class List;
}

namespace video {
class Camera;
}

namespace render {
class Shadow;
}

namespace frontend {

class ClientEntity;

class ClientEntityRenderer : public core::IComponent {
private:
	shader::SkeletonShader _chrShader;
	shader::SkeletonData _materialBlock;
	shader::SkeletonshadowmapShader& _skeletonShadowMapShader;
	shader::SkeletondepthmapShader& _skeletondepthmapShader;

	video::FrameBuffer _entitiesDepthBuffer;

	float _viewDistance = 0.0f;
	float _fogRange = 0.0f;
	float _seconds = 0.0f;

	glm::vec3 _focusPos { 0.0f };

	core::VarPtr _shadowMap;
public:
	ClientEntityRenderer();
	virtual ~ClientEntityRenderer() = default;

	void construct() override;
	bool init() override;
	void shutdown() override;

	void update(const glm::vec3& focusPos, float seconds);

	void bindEntitiesDepthBuffer(video::TextureUnit texunit);

	int renderEntitiesToDepthMap(const core::List<ClientEntity*>& entities, const glm::mat4& viewProjectionMatrix);
	int renderEntities(const core::List<ClientEntity*>& entities, const glm::mat4& viewProjectionMatrix, const glm::vec4& clipPlane, const render::Shadow& shadow);
	int renderEntityDetails(const core::List<ClientEntity*>& entities, const video::Camera& camera);

	void setViewDistance(float viewDistance, float fogRange);
	video::FrameBuffer &entitiesBuffer();
	void renderShadows(const core::List<ClientEntity*>& entities, render::Shadow& shadow);
};

inline void ClientEntityRenderer::setViewDistance(float viewDistance, float fogRange) {
	_viewDistance = viewDistance;
	_fogRange = fogRange;
}

inline video::FrameBuffer &ClientEntityRenderer::entitiesBuffer() {
	return _entitiesDepthBuffer;
}

}
