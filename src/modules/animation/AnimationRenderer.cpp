/**
 * @file
 */

#include "AnimationRenderer.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "core/GLM.h"
#include "core/ArrayLength.h"
#include "voxel/MaterialColor.h"
#include "animation/Vertex.h"
#include "video/Renderer.h"
#include "video/Shader.h"
#include "RenderShaders.h"
#include "video/Camera.h"

namespace animation {

AnimationRenderer::AnimationRenderer() :
		_shader(shader::SkeletonShader::getInstance()),
		_shadowMapShader(shader::SkeletonshadowmapShader::getInstance()) {
}

bool AnimationRenderer::init() {
	if (!_shader.setup()) {
		Log::error("Failed to setup character shader");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return false;
	}
	render::ShadowParameters shadowParams;
	shadowParams.maxDepthBuffers = shader::SkeletonShaderConstants::getMaxDepthBuffers();
	if (!_shadow.init(shadowParams)) {
		Log::error("Failed to init shadow object");
		return false;
	}

	const int shaderMaterialColorsArraySize = lengthof(shader::SkeletonData::MaterialblockData::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::SkeletonData::MaterialblockData materialBlock;
	core_memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	if (!_shaderData.create(materialBlock)) {
		Log::error("Failed to create material buffer");
		return false;
	}

	_vertices = _vbo.create();
	_indices = _vbo.create(nullptr, 0, video::BufferType::IndexBuffer);

	_vbo.addAttribute(_shader.getPosAttribute(_vertices, &Vertex::pos));
	video::Attribute color = _shader.getColorIndexAttribute(_vertices, &Vertex::colorIndex);
	color.typeIsInt = true;
	_vbo.addAttribute(color);
	video::Attribute boneId = _shader.getBoneIdAttribute(_vertices, &Vertex::boneId);
	boneId.typeIsInt = true;
	_vbo.addAttribute(boneId);
	video::Attribute ambientOcclusion = _shader.getAmbientOcclusionAttribute(_vertices, &Vertex::ambientOcclusion);
	ambientOcclusion.typeIsInt = true;
	_vbo.addAttribute(ambientOcclusion);

	return true;
}

void AnimationRenderer::shutdown() {
	_shaderData.shutdown();
	_shader.shutdown();
	_shadowMapShader.shutdown();
	_vbo.shutdown();
	_shadow.shutdown();
	_vertices = -1;
	_indices = -1;
}

void AnimationRenderer::render(const AnimationEntity& character, const video::Camera& camera) {
	const Indices& i = character.indices();
	const Vertices& v = character.vertices();
	core_assert_always(_vbo.update(_indices, &i.front(), i.size() * sizeof(IndexType)));
	core_assert_always(_vbo.update(_vertices, &v.front(), v.size() * sizeof(Vertex)));
	const uint32_t numIndices = _vbo.elements(_indices, 1, sizeof(IndexType));
	if (numIndices == 0u) {
		return;
	}
	glm::mat4 bones[shader::SkeletonShaderConstants::getMaxBones()];
	const AnimationSettings& settings = character.animationSettings();
	const Skeleton& skeleton = character.skeleton();
	skeleton.update(settings, bones);

	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	_shadow.update(camera, true);

	_shadowMapShader.activate();
	_vbo.bind();
	core_assert_always(_shadowMapShader.setBones(bones));
	_shadowMapShader.setModel(glm::mat4(1.0f));
	_shadow.render([&] (int index, const glm::mat4& lightViewProjection) {
		_shadowMapShader.setLightviewprojection(lightViewProjection);
		video::drawElements<IndexType>(video::Primitive::Triangles, numIndices);
		return true;
	});
	_vbo.unbind();
	_shadowMapShader.deactivate();

	video::ScopedShader scopedShader(_shader);
	_vbo.bind();
	core_assert_always(_shader.setBones(bones));
	video::clearColor(_clearColor);
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);

	_shadow.bind(video::TextureUnit::One);

	_shader.setMaterialblock(_shaderData.getMaterialblockUniformBuffer());

	_shader.setModel(glm::mat4(1.0f));
	_shader.setViewprojection(camera.viewProjectionMatrix());

	_shader.setLightdir(_shadow.sunDirection());
	_shader.setDiffuseColor(_diffuseColor);
	_shader.setAmbientColor(_ambientColor);
	_shader.setNightColor(_nightColor);
	_shader.setTime(_seconds);

	// clip plane is disabled here
	_shader.setClipplane(glm::vec4(glm::down, 1000.0f));

	_shader.setFocuspos(camera.target());
	_shader.setFogcolor(_clearColor);
	_shader.setFogrange(_fogRange);

	_shader.setShadowmap(video::TextureUnit::One);
	_shader.setDepthsize(glm::vec2(_shadow.dimension()));
	_shader.setCascades(_shadow.cascades());
	_shader.setDistances(_shadow.distances());

	video::drawElements<IndexType>(video::Primitive::Triangles, numIndices);
	_vbo.unbind();
}

}
