/**
 * @file
 */

#include "ClientEntityRenderer.h"
#include "video/Renderer.h"
#include "frontend/ClientEntity.h"
#include "frontend/Colors.h"
#include "video/Trace.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/collection/List.h"
#include "core/ArrayLength.h"
#include "voxel/MaterialColor.h"
#include "video/Camera.h"
#include "video/ScopedState.h"
#include "render/Shadow.h"
#include "core/Trace.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace frontend {

ClientEntityRenderer::ClientEntityRenderer() :
		_skeletonShadowMapShader(shader::SkeletonshadowmapShader::getInstance()),
		_skeletondepthmapShader(shader::SkeletondepthmapShader::getInstance()) {
}

void ClientEntityRenderer::construct() {
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
}

bool ClientEntityRenderer::init() {
	if (!_chrShader.setup()) {
		Log::error("Failed to setup the post skeleton shader");
		return false;
	}
	if (!_skeletonShadowMapShader.setup()) {
		Log::error("Failed to init skeleton shadowmap shader");
		return false;
	}
	if (!_skeletondepthmapShader.setup()) {
		Log::error("Failed to init skeleton depthmap shader");
		return false;
	}
	const int shaderMaterialColorsArraySize = lengthof(shader::SkeletonData::MaterialblockData::materialcolor);
	const int materialColorsArraySize = (int)voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::SkeletonData::MaterialblockData materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.create(materialBlock);

	video::TextureConfig textureCfg;
	textureCfg.format(video::TextureFormat::D32F);
	textureCfg.compareFunc(video::CompareFunc::Less).compareMode(video::TextureCompareMode::RefToTexture);
	textureCfg.borderColor(glm::vec4(1.0f));
	textureCfg.wrap(video::TextureWrap::ClampToBorder);

	video::FrameBufferConfig entitesDepthCfg;
	entitesDepthCfg.dimension(glm::ivec2(1024, 1024)).colorTexture(false);
	entitesDepthCfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Depth);
	if (!_entitiesDepthBuffer.init(entitesDepthCfg)) {
		Log::error("Failed to initialize the entity depth buffer");
		return false;
	}

	return true;
}

void ClientEntityRenderer::shutdown() {
	_chrShader.shutdown();
	_skeletonShadowMapShader.shutdown();
	_skeletondepthmapShader.shutdown();
	_entitiesDepthBuffer.shutdown();
}

void ClientEntityRenderer::update(const glm::vec3& focusPos, float seconds) {
	_focusPos = focusPos;
	_seconds = seconds;
}

void ClientEntityRenderer::renderShadows(const core::List<ClientEntity*>& entities, render::Shadow& shadow) {
	core_trace_scoped(RenderEntityShadows);
	_skeletonShadowMapShader.activate();
	shadow.render([this, entities] (int i, const glm::mat4& lightViewProjection) {
		_skeletonShadowMapShader.setLightviewprojection(lightViewProjection);
		for (const auto& ent : entities) {
			_skeletonShadowMapShader.setBones(ent->bones()._items);
			_skeletonShadowMapShader.setModel(ent->modelMatrix());
			const uint32_t numIndices = ent->bindVertexBuffers(_chrShader);
			video::drawElements<animation::IndexType>(video::Primitive::Triangles, numIndices);
			ent->unbindVertexBuffers();
		}
		return true;
	}, true);
	_skeletonShadowMapShader.deactivate();
}

int ClientEntityRenderer::renderEntityDetails(const core::List<ClientEntity*>& entities, const video::Camera& camera) {
	if (entities.empty()) {
		return 0;
	}
	video_trace_scoped(RenderEntityDetails);
	alignas(16) static const struct {
		glm::vec3 start;
		glm::vec3 end;
		glm::vec4 color;
	} verticesAxis[] = {
		{glm::vec3(0.0f), glm::right, core::Color::Red},
		{glm::vec3(0.0f), glm::up, core::Color::Green},
		{glm::vec3(0.0f), glm::forward, core::Color::Blue}
	};
	int drawCallsEntities = 0;
#if 0
	for (frontend::ClientEntity* ent : entities) {
		const glm::mat4 model = glm::rotate(glm::translate(ent->position()), ent->orientation(), glm::up);
		// TODO: draw health bar
		// TODO: draw debug orientation
	}
#endif
	return drawCallsEntities;
}

void ClientEntityRenderer::bindEntitiesDepthBuffer(video::TextureUnit texunit) {
	video::bindTexture(texunit, _entitiesDepthBuffer, video::FrameBufferAttachment::Depth);
}

int ClientEntityRenderer::renderEntitiesToDepthMap(const core::List<ClientEntity*>& entities, const glm::mat4& viewProjectionMatrix) {
	video_trace_scoped(RenderEntitiesToDepthMap);
	_entitiesDepthBuffer.bind(true);
	video::colorMask(false, false, false, false);

	video::ScopedState blend(video::State::Blend, false);
	video::ScopedShader scoped(_skeletondepthmapShader);
	_skeletondepthmapShader.setViewprojection(viewProjectionMatrix);
	for (const auto& ent : entities) {
		_skeletondepthmapShader.setBones(ent->bones()._items);
		_skeletondepthmapShader.setModel(ent->modelMatrix());
		const uint32_t numIndices = ent->bindVertexBuffers(_chrShader);
		video::drawElements<animation::IndexType>(video::Primitive::Triangles, numIndices);
		ent->unbindVertexBuffers();
	}

	video::colorMask(true, true, true, true);
	_entitiesDepthBuffer.unbind();
	return 0;
}

int ClientEntityRenderer::renderEntities(const core::List<ClientEntity*>& entities, const glm::mat4& viewProjectionMatrix, const glm::vec4& clipPlane, const render::Shadow& shadow) {
	if (entities.empty()) {
		return 0;
	}
	video_trace_scoped(ClientEntityRendererEntities);

	int drawCallsEntities = 0;

	video::enable(video::State::DepthTest);
	video::ScopedShader scoped(_chrShader);

	if (_chrShader.isDirty()) {
		_chrShader.setDiffuseColor(diffuseColor);
		_chrShader.setAmbientColor(ambientColor);
		_chrShader.setFogcolor(clearColor);
		_chrShader.setNightColor(nightColor);
		_chrShader.setMaterialblock(_materialBlock);
		_chrShader.setShadowmap(video::TextureUnit::One);
		_chrShader.markClean();
	}
	_chrShader.setFogrange(_fogRange);
	_chrShader.setFocuspos(_focusPos);
	_chrShader.setLightdir(shadow.sunDirection());
	_chrShader.setTime(_seconds);
	_chrShader.setClipplane(clipPlane);
	_chrShader.setViewprojection(viewProjectionMatrix);

	const bool shadowMap = _shadowMap->boolVal();
	if (shadowMap) {
		_chrShader.setDepthsize(glm::vec2(shadow.dimension()));
		_chrShader.setCascades(shadow.cascades());
		_chrShader.setDistances(shadow.distances());
	}
	for (frontend::ClientEntity* ent : entities) {
		// TODO: apply the clipping plane to the entity frustum culling
		_chrShader.setModel(ent->modelMatrix());
		core_assert_always(_chrShader.setBones(ent->bones()._items));
		const uint32_t numIndices = ent->bindVertexBuffers(_chrShader);
		++drawCallsEntities;
		video::drawElements<animation::IndexType>(video::Primitive::Triangles, numIndices);
		ent->unbindVertexBuffers();
	}
	return drawCallsEntities;
}

}