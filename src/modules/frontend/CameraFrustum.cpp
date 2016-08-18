#include "CameraFrustum.h"
#include "video/GLFunc.h"
#include "core/AABB.h"

namespace frontend {

bool CameraFrustum::init(const video::Camera& frustumCamera, const glm::vec4& color) {
	if (!_colorShader.setup()) {
		return false;
	}

	// +2 because we also show the position and target as line
	constexpr int outSize = video::FRUSTUM_VERTICES_MAX + 2;
	glm::vec4 colors[outSize];
	uint32_t indices[video::FRUSTUM_VERTICES_MAX * 3 + 2];

	frustumCamera.frustumCorners(nullptr, indices);

	for (size_t v = 0; v < SDL_arraysize(colors); ++v) {
		colors[v] = color;
	}
	colors[video::FRUSTUM_VERTICES_MAX + 0] = core::Color::Green;
	colors[video::FRUSTUM_VERTICES_MAX + 1] = core::Color::Green;

	indices[video::FRUSTUM_VERTICES_MAX * 3 + 0] = video::FRUSTUM_VERTICES_MAX + 0;
	indices[video::FRUSTUM_VERTICES_MAX * 3 + 1] = video::FRUSTUM_VERTICES_MAX + 1;

	// upload to gpu
	_vertexIndex = _frustumBuffer.create(nullptr, 0);
	_indexIndex = _frustumBuffer.create(indices, sizeof(indices), GL_ELEMENT_ARRAY_BUFFER);
	const int32_t cIndex = _frustumBuffer.create(colors, sizeof(colors));

	// configure shader attributes
	_frustumBuffer.addAttribute(_colorShader.getLocationPos(), _vertexIndex, 4);
	_frustumBuffer.addAttribute(_colorShader.getLocationColor(), cIndex, 4);

	_vertexAABBIndex = _aabbBuffer.create(nullptr, 0);
	// we don't want the last two to be uploaded - they are not used in the aabb
	// Note: this is more or less a hack that assumes that the indices are the same for
	// the frustum and the aabb
	_indexAABBIndex = _aabbBuffer.create(indices, sizeof(indices) - 2 * sizeof(uint32_t), GL_ELEMENT_ARRAY_BUFFER);
	const int32_t cAABBIndex = _aabbBuffer.create(colors, sizeof(colors));

	// configure shader attributes
	_aabbBuffer.addAttribute(_colorShader.getLocationPos(), _vertexAABBIndex, 4);
	_aabbBuffer.addAttribute(_colorShader.getLocationColor(), cAABBIndex, 4);

	return true;
}

void CameraFrustum::shutdown() {
	_colorShader.shutdown();
	_frustumBuffer.shutdown();
	_aabbBuffer.shutdown();
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

	core_assert_always(_frustumBuffer.update(_vertexIndex, out4, sizeof(out4)));
	core_assert_always(_frustumBuffer.bind());
	const GLuint indices = _frustumBuffer.elements(_indexIndex, 1, sizeof(uint32_t));
	glDrawElements(GL_LINES, indices, GL_UNSIGNED_INT, 0);
	_frustumBuffer.unbind();

	if (!_renderAABB) {
		return;
	}

	core::AABB<float> aabb(out[1], out[6]);

	glm::vec3 aabbOut[8];
	aabb.corners(aabbOut, nullptr);
	glm::vec4 aabbOut4[SDL_arraysize(aabbOut)];
	const glm::mat4& transform = glm::inverse(frustumCamera.projectionMatrix() * frustumCamera.viewMatrix());
	for (size_t i = 0; i < SDL_arraysize(aabbOut); ++i) {
		aabbOut4[i] = transform * glm::vec4(aabbOut[i], 1.0f);
	}

	core_assert_always(_aabbBuffer.update(_vertexAABBIndex, aabbOut4, sizeof(aabbOut4)));
	core_assert_always(_aabbBuffer.bind());
	const GLuint aabbIndicesAmount = _aabbBuffer.elements(_indexAABBIndex, 1, sizeof(uint32_t));
	core_assert(aabbIndicesAmount == SDL_arraysize(aabbOut) * 3);
	glDrawElements(GL_LINES, aabbIndicesAmount, GL_UNSIGNED_INT, 0);
	_aabbBuffer.unbind();
	GL_checkError();
}

}
