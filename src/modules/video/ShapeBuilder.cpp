#include "ShapeBuilder.h"

namespace video {

void ShapeBuilder::aabb(const core::AABB<float>& aabb) {
	reserve(8);

	static const glm::vec3 vecs[8] = {
		glm::vec3(-0.5f,  0.5f,  0.5f), glm::vec3(-0.5f, -0.5f,  0.5f),
		glm::vec3( 0.5f,  0.5f,  0.5f), glm::vec3( 0.5f, -0.5f,  0.5f),
		glm::vec3(-0.5f,  0.5f, -0.5f), glm::vec3(-0.5f, -0.5f, -0.5f),
		glm::vec3( 0.5f,  0.5f, -0.5f), glm::vec3( 0.5f, -0.5f, -0.5f)
	};
	const glm::vec3& width = aabb.getWidth() / 2.0f;
	const glm::vec3& center = aabb.getCenter();
	for (int i = 0; i < 8; ++i) {
		addVertex(vecs[i] * width + center, glm::zero<glm::vec2>());
	}

	uint32_t currentIndex = 0;

	// front
	_indices[currentIndex++] = 0;
	_indices[currentIndex++] = 1;

	_indices[currentIndex++] = 1;
	_indices[currentIndex++] = 3;

	_indices[currentIndex++] = 3;
	_indices[currentIndex++] = 2;

	_indices[currentIndex++] = 2;
	_indices[currentIndex++] = 0;

	// back
	_indices[currentIndex++] = 4;
	_indices[currentIndex++] = 5;

	_indices[currentIndex++] = 5;
	_indices[currentIndex++] = 7;

	_indices[currentIndex++] = 7;
	_indices[currentIndex++] = 6;

	_indices[currentIndex++] = 6;
	_indices[currentIndex++] = 4;

	// connections
	_indices[currentIndex++] = 0;
	_indices[currentIndex++] = 4;

	_indices[currentIndex++] = 2;
	_indices[currentIndex++] = 6;

	_indices[currentIndex++] = 1;
	_indices[currentIndex++] = 5;

	_indices[currentIndex++] = 3;
	_indices[currentIndex++] = 7;
}

void ShapeBuilder::frustum(const Camera& camera) {
	reserve(video::FRUSTUM_VERTICES_MAX, 2);
	camera.frustumCorners(&_vertices[0], &_indices[0]);
	_indices[video::FRUSTUM_VERTICES_MAX * 3 + 0] = video::FRUSTUM_VERTICES_MAX + 0;
	_indices[video::FRUSTUM_VERTICES_MAX * 3 + 1] = video::FRUSTUM_VERTICES_MAX + 1;
}

void ShapeBuilder::plane(uint32_t tesselation) {
	static const glm::vec2 uv0(0.0f, 1.0f);
	static const glm::vec2 uv1(1.0f, 0.0f);
	static const glm::vec2 uv2(0.0f, 0.0f);
	static const glm::vec2 meshBounds(uv1.x - uv0.x, uv2.y - uv0.y);
	static const glm::vec2 uvBounds(uv1.x - uv0.y, uv0.y - uv2.y);
	static const glm::vec2 uvPos = uv2;
	static const glm::vec2 anchorOffset(meshBounds.x / 2, meshBounds.y / 2);

	const uint32_t strucWidth = tesselation + 2;
	const float segmentWidth = 1.0f / (tesselation + 1);
	const float scaleX = meshBounds.x / (tesselation + 1);
	const float scaleY = meshBounds.y / (tesselation + 1);

	reserve(strucWidth * strucWidth);

	for (float y = 0.0f; y < strucWidth; ++y) {
		for (float x = 0.0f; x < strucWidth; ++x) {
			const glm::vec2 uv((x * segmentWidth * uvBounds.x) + uvPos.x, uvBounds.y - (y * segmentWidth * uvBounds.y) + uvPos.y);
			const glm::vec3 v(x * scaleX - anchorOffset.x, 0.0f, y * scaleY - anchorOffset.y);
			addVertex(v, uv);
		}
	}

	for (int y = 0; y < tesselation + 1; ++y) {
		for (int x = 0; x < tesselation + 1; ++x) {
			_indices.emplace_back((y * strucWidth) + x);
			_indices.emplace_back((y * strucWidth) + x + 1);
			_indices.emplace_back(((y + 1) * strucWidth) + x);
			_indices.emplace_back(((y + 1) * strucWidth) + x);
			_indices.emplace_back((y * strucWidth) + x + 1);
			_indices.emplace_back(((y + 1) * strucWidth) + x + 1);
		}
	}
}

void ShapeBuilder::shutdown() {
	clear();
}

}
