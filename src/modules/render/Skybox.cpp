/**
 * @file
 */

#include "Skybox.h"
#include "video/ScopedState.h"
#include "video/Shader.h"
#include "core/Log.h"

namespace render {

bool Skybox::init(const char *filename) {
	core_assert(_shader.getComponentsPos() == 3);
	if (!_cubemap.init(filename)) {
		Log::error("Failed to initialize the cubemap");
		return false;
	}

	if (!_shader.setup()) {
		Log::error("Failed to initialize the skybox shader");
		_cubemap.shutdown();
		return false;
	}

	_skyboxIndex = _vbo.createSkyboxQuad();
	if (_skyboxIndex == -1) {
		Log::error("Failed to initialize the vertex buffer");
		shutdown();
		return false;
	}
	if (!_vbo.addAttribute(_shader.getPosAttribute(_skyboxIndex, &glm::vec3::x))) {
		Log::error("Failed to initialize vertex buffer attributes");
		shutdown();
		return false;
	}
	return true;
}

void Skybox::shutdown() {
	_cubemap.shutdown();
	_vbo.shutdown();
	_shader.shutdown();
	_skyboxIndex = -1;
}

void Skybox::bind(video::TextureUnit unit) {
	_cubemap.bind(unit);
}

void Skybox::unbind(video::TextureUnit unit) {
	_cubemap.unbind(unit);
}

void Skybox::render(const video::Camera& camera) {
	if (_skyboxIndex == -1) {
		return;
	}
	const video::CompareFunc func = video::getDepthFunc();
	video::depthFunc(video::CompareFunc::LessEqual);
	video::ScopedShader scopedshader(_shader);
	video::ScopedBuffer scopedbuffer(_vbo);
	_shader.setProjection(camera.projectionMatrix());
	const glm::mat4 viewWithoutTranslation(glm::mat3(camera.viewMatrix()));
	_shader.setView(viewWithoutTranslation);
	_cubemap.bind();
	video::drawArrays(video::Primitive::Triangles, _vbo.elements(_skyboxIndex));
	_cubemap.unbind();
	video::depthFunc(func);
}
}
