#pragma once

#include "video/VertexBuffer.h"
#include "video/Camera.h"
#include "video/ShapeBuilder.h"
#include "core/Color.h"
#include "ColorShader.h"
#include <algorithm>

namespace frontend {

class Plane {
private:
	shader::ColorShader _colorShader;
	video::VertexBuffer _planeBuffer;
	video::ShapeBuilder _shapeBuilder;
	int32_t _iIndex = -1;
public:
	void render(const video::Camera& camera) {
		video::ScopedShader scoped(_colorShader);
		_colorShader.setView(camera.viewMatrix());
		_colorShader.setProjection(camera.projectionMatrix());
		core_assert_always(_planeBuffer.bind());
		const GLuint indices = _planeBuffer.elements(_iIndex, 1, sizeof(video::ShapeBuilder::Indices::value_type));
		glDrawElements(GL_TRIANGLES, indices, GL_UNSIGNED_INT, 0);
		_planeBuffer.unbind();
		GL_checkError();
	}

	void shutdown() {
		_colorShader.shutdown();
		_planeBuffer.shutdown();
		_shapeBuilder.shutdown();
	}

	/**
	 * @param[in] position The offset that should be applied to the center of the plane
	 * @param[in] tesselation The amount of splits on the plane that should be made
	 * @param[in] scale The vertices are in the normalized coordinate space between -0.5 and 0.5 - we have to scale them up to the size we need
	 * @param[in] color The color of the plane.
	 */
	bool init(const glm::vec3& position, int tesselation = 0, float scale = 100.0f, const glm::vec4& color = core::Color::White) {
		if (!_colorShader.setup()) {
			return false;
		}

		_shapeBuilder.initPlane(tesselation);

		const video::ShapeBuilder::Vertices& vertices = _shapeBuilder.getVertices();
		const video::ShapeBuilder::Indices& indices = _shapeBuilder.getIndices();

		std::vector<glm::vec4> verticesPlane;
		verticesPlane.reserve(vertices.size());
		for (const glm::vec2& v : vertices) {
			verticesPlane.emplace_back(position.x + v.x * scale, position.y, position.z + v.y * scale, 1.0f);
		}

		std::vector<glm::vec4> colorPlane(vertices.size());
		std::fill(colorPlane.begin(), colorPlane.end(), color);

		const size_t vecSize = vertices.size() * sizeof(glm::vec4);

		const int32_t vIndex = _planeBuffer.create(&verticesPlane[0], vecSize);
		const int32_t cIndex = _planeBuffer.create(&colorPlane[0], vecSize);
		_iIndex = _planeBuffer.create(&indices[0], indices.size() * sizeof(video::ShapeBuilder::Indices::value_type), GL_ELEMENT_ARRAY_BUFFER);
		core_assert(vIndex >= 0 && cIndex >= 0 && _iIndex >= 0);
		_planeBuffer.addAttribute(_colorShader.getLocationPos(), vIndex, 4);
		_planeBuffer.addAttribute(_colorShader.getLocationColor(), cIndex, 4);
		return true;
	};
};

}
