#pragma once

#include "video/VertexBuffer.h"
#include "video/Camera.h"
#include "core/Color.h"
#include "ColorShader.h"

namespace frontend {

class Plane {
private:
	shader::ColorShader _colorShader;
	video::VertexBuffer _planeBuffer;
public:
	void render(const video::Camera& camera) {
		video::ScopedShader scoped(_colorShader);
		_colorShader.setView(camera.viewMatrix());
		_colorShader.setProjection(camera.projectionMatrix());
		core_assert_always(_planeBuffer.bind());
		glDrawArrays(GL_TRIANGLES, 0, _planeBuffer.elements(0, 4));
		_planeBuffer.unbind();
		GL_checkError();
	}

	void shutdown() {
		_colorShader.shutdown();
		_planeBuffer.shutdown();
	}

	bool init(const glm::vec4& color = core::Color::Red) {
		if (!_colorShader.setup()) {
			return false;
		}

		static const float colorAxis[] = {
				color.r, color.g, color.b, color.a,
				color.r, color.g, color.b, color.a,
				color.r, color.g, color.b, color.a,
				color.r, color.g, color.b, color.a};

		const int32_t vIndex = _planeBuffer.createPlane(2, 2);
		const int32_t cIndex = _planeBuffer.create(colorAxis, sizeof(colorAxis));
		core_assert_msg(_planeBuffer.elements(0, 4) == _planeBuffer.elements(1, 4),
				"%i (0) versus %i (1)", _planeBuffer.elements(0, 4), _planeBuffer.elements(1, 4));
		core_assert(vIndex >= 0 && cIndex >= 0);
		_planeBuffer.addAttribute(_colorShader.getLocationPos(), vIndex, 4);
		_planeBuffer.addAttribute(_colorShader.getLocationColor(), cIndex, 4);
		return true;
	};
};

}
