#include "CameraFrustum.h"

namespace frontend {

bool CameraFrustum::init(const video::Camera& frustumCamera, const glm::vec4& color) {
	if (!_colorShader.setup()) {
		return false;
	}

	// allocate buffer space
	glm::vec3 out[video::FRUSTUM_VERTICES_MAX];
	// +2 because we also show the position and target as line
	constexpr int outSize = video::FRUSTUM_VERTICES_MAX + 2;
	glm::vec4 out4[outSize];
	glm::vec4 colors[outSize];
	uint32_t indices[24 + 2];

	frustumCamera.frustumCorners(out, indices);

	// fill buffers
	for (size_t i = 0; i < SDL_arraysize(out); ++i) {
		out4[i] = glm::vec4(out[i], 1.0f);
	}

	for (size_t v = 0; v < SDL_arraysize(colors); ++v) {
		colors[v] = color;
	}
	colors[video::FRUSTUM_VERTICES_MAX + 0] = core::Color::Green;
	colors[video::FRUSTUM_VERTICES_MAX + 1] = core::Color::Green;
	out4[video::FRUSTUM_VERTICES_MAX + 0] = glm::vec4(frustumCamera.position(), 1.0f);
	out4[video::FRUSTUM_VERTICES_MAX + 1] = glm::vec4(frustumCamera.target(), 1.0f);

	indices[24 + 0] = video::FRUSTUM_VERTICES_MAX + 0;
	indices[24 + 1] = video::FRUSTUM_VERTICES_MAX + 1;

	// upload to gpu
	_vertexIndex = _frustumBuffer.create(out4, sizeof(out4));
	_indexIndex = _frustumBuffer.create(indices, sizeof(indices), GL_ELEMENT_ARRAY_BUFFER);
	const int32_t cIndex = _frustumBuffer.create(colors, sizeof(colors));

	// configure shader attributes
	_frustumBuffer.addAttribute(_colorShader.getLocationPos(), _vertexIndex, 4);
	_frustumBuffer.addAttribute(_colorShader.getLocationColor(), cIndex, 4);

	return true;
}

void CameraFrustum::shutdown() {
	_colorShader.shutdown();
	_frustumBuffer.shutdown();
}

void CameraFrustum::render(const video::Camera& camera, const video::Camera& frustumCamera) {
	video::ScopedShader scoped(_colorShader);
	_colorShader.setView(camera.viewMatrix());
	_colorShader.setProjection(camera.projectionMatrix());

	glm::vec3 out[video::FRUSTUM_VERTICES_MAX];
	frustumCamera.frustumCorners(out, nullptr);

	glm::vec4 out4[video::FRUSTUM_VERTICES_MAX + 2];
	for (size_t i = 0; i < SDL_arraysize(out); ++i) {
		out4[i] = glm::vec4(out[i], 1.0f);
	}
	out4[video::FRUSTUM_VERTICES_MAX + 0] = glm::vec4(frustumCamera.position(), 1.0f);
	out4[video::FRUSTUM_VERTICES_MAX + 1] = glm::vec4(frustumCamera.target(), 1.0f);
	_frustumBuffer.update(_vertexIndex, out4, sizeof(out4));

	core_assert_always(_frustumBuffer.bind());
	const GLuint indices = _frustumBuffer.elements(_indexIndex, 1, sizeof(uint32_t));
	glDrawElements(GL_LINES, indices, GL_UNSIGNED_INT, 0);
	_frustumBuffer.unbind();
	GL_checkError();
}

}
