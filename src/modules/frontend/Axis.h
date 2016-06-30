#pragma once

#include "video/VertexBuffer.h"
#include "video/Camera.h"
#include "ColorShader.h"

namespace frontend {

class Axis {
private:
	shader::ColorShader _colorShader;
	video::VertexBuffer _axisBuffer;
public:
	void render(const video::Camera& camera) {
		video::ScopedShader scoped(_colorShader);
		_colorShader.setView(camera.viewMatrix());
		_colorShader.setProjection(camera.projectionMatrix());
		// TODO: add x, y and z letters to the axis
		glDisable(GL_DEPTH_TEST);
		core_assert_always(_axisBuffer.bind());
		glLineWidth(4.0f);
		glDrawArrays(GL_LINES, 0, 6);
		glLineWidth(1.0f);
		_axisBuffer.unbind();
		GL_checkError();
	}

	void shutdown() {
		_colorShader.shutdown();
		_axisBuffer.shutdown();
	}

	bool init() {
		if (!_colorShader.setup()) {
			return false;
		}

		static const float verticesAxis[] = {
				  0.0f,   0.0f,   0.0f, 1.0f,
				 20.0f,   0.0f,   0.0f, 1.0f,
				  0.0f,   0.0f,   0.0f, 1.0f,
				  0.0f,  20.0f,   0.0f, 1.0f,
				  0.0f,   0.0f,   0.0f, 1.0f,
				  0.0f,   0.0f,  20.0f, 1.0f};

		static const float colorAxis[] = {
				1.0f, 0.0f, 0.0f, 1.0f,
				1.0f, 0.0f, 0.0f, 1.0f,
				0.0f, 1.0f, 0.0f, 1.0f,
				0.0f, 1.0f, 0.0f, 1.0f,
				0.0f, 0.0f, 1.0f, 1.0f,
				0.0f, 0.0f, 1.0f, 1.0f};

		const int32_t vIndex = _axisBuffer.create(verticesAxis, sizeof(verticesAxis));
		const int32_t cIndex = _axisBuffer.create(colorAxis, sizeof(colorAxis));
		core_assert(vIndex >= 0 && cIndex >= 0);
		_axisBuffer.addAttribute(_colorShader.getLocationPos(), vIndex, 4);
		_axisBuffer.addAttribute(_colorShader.getLocationColor(), cIndex, 4);
		return true;
	};
};

}
