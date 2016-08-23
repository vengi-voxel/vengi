#include "ShapeBuilder.h"

namespace video {

// TODO: convert everything into triangles - if you wanna have lines, use glPolygonMode

void ShapeBuilder::aabb(const core::AABB<float>& aabb) {
	static const glm::vec3 vecs[8] = {
		glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3(-1.0f, -1.0f,  1.0f),
		glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec3( 1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec3( 1.0f, -1.0f, -1.0f)
	};
	reserve(SDL_arraysize(vecs));
	const glm::vec3& width = aabb.getWidth() / 2.0f;
	const glm::vec3& center = aabb.getCenter();
	for (size_t i = 0; i < SDL_arraysize(vecs); ++i) {
		addVertex(vecs[i] * width + center, glm::zero<glm::vec2>());
	}

	// front
	_indices.push_back(0);
	_indices.push_back(1);

	_indices.push_back(1);
	_indices.push_back(3);

	_indices.push_back(3);
	_indices.push_back(2);

	_indices.push_back(2);
	_indices.push_back(0);

	// back
	_indices.push_back(4);
	_indices.push_back(5);

	_indices.push_back(5);
	_indices.push_back(7);

	_indices.push_back(7);
	_indices.push_back(6);

	_indices.push_back(6);
	_indices.push_back(4);

	// connections
	_indices.push_back(0);
	_indices.push_back(4);

	_indices.push_back(2);
	_indices.push_back(6);

	_indices.push_back(1);
	_indices.push_back(5);

	_indices.push_back(3);
	_indices.push_back(7);
}

void ShapeBuilder::frustum(const Camera& camera) {
	reserve(video::FRUSTUM_VERTICES_MAX + 2);
	glm::vec3 out[video::FRUSTUM_VERTICES_MAX];
	uint32_t indices[video::FRUSTUM_VERTICES_MAX * 3];
	camera.frustumCorners(out, indices);

	for (size_t i = 0; i < SDL_arraysize(out); ++i) {
		addVertex(out[i], glm::zero<glm::vec2>());
	}

	setColor(core::Color::Green);
	addVertex(camera.position(), glm::zero<glm::vec2>());
	addVertex(camera.target(), glm::zero<glm::vec2>());

	for (size_t i = 0; i < SDL_arraysize(indices); ++i) {
		_indices.push_back(indices[i]);
	}
	_indices.push_back(video::FRUSTUM_VERTICES_MAX + 0);
	_indices.push_back(video::FRUSTUM_VERTICES_MAX + 1);
}

void ShapeBuilder::axis(float scale) {
	const glm::vec3 verticesAxis[] = {
			 glm::vec3( 0.0f,   0.0f,   0.0f),
			 glm::vec3(scale,   0.0f,   0.0f),
			 glm::vec3( 0.0f,   0.0f,   0.0f),
			 glm::vec3( 0.0f,  scale,   0.0f),
			 glm::vec3( 0.0f,   0.0f,   0.0f),
			 glm::vec3( 0.0f,   0.0f,  scale)};

	setColor(core::Color::Red);
	addVertex(verticesAxis[0], glm::zero<glm::vec2>());
	addVertex(verticesAxis[1], glm::zero<glm::vec2>());
	setColor(core::Color::Green);
	addVertex(verticesAxis[2], glm::zero<glm::vec2>());
	addVertex(verticesAxis[3], glm::zero<glm::vec2>());
	setColor(core::Color::Blue);
	addVertex(verticesAxis[4], glm::zero<glm::vec2>());
	addVertex(verticesAxis[5], glm::zero<glm::vec2>());

	for (size_t i = 0; i < SDL_arraysize(verticesAxis); ++i) {
		_indices.push_back(i);
	}
}

void ShapeBuilder::plane(uint32_t tesselation, float scale) {
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
			addVertex(v * scale, uv);
		}
	}

	for (size_t y = 0; y < tesselation + 1; ++y) {
		for (size_t x = 0; x < tesselation + 1; ++x) {
			_indices.emplace_back((y * strucWidth) + x);
			_indices.emplace_back((y * strucWidth) + x + 1);
			_indices.emplace_back(((y + 1) * strucWidth) + x);
			_indices.emplace_back(((y + 1) * strucWidth) + x);
			_indices.emplace_back((y * strucWidth) + x + 1);
			_indices.emplace_back(((y + 1) * strucWidth) + x + 1);
		}
	}
}

void ShapeBuilder::sphere(int numSlices, int numStacks, float radius) {
	static constexpr float pi = glm::pi<float>();
	static constexpr float twoPi = glm::two_pi<float>();
	const float du = 1.0f / numSlices;
	const float dv = 1.0f / numStacks;

	for (int stack = 0; stack <= numStacks; stack++) {
		const float stackAngle = (pi * stack) / numStacks;
		const float sinStack = glm::sin(stackAngle);
		const float cosStack = glm::cos(stackAngle);
		for (int slice = 0; slice <= numSlices; slice++) {
			const float sliceAngle = (twoPi * slice) / numSlices;
			const float sinSlice = glm::sin(sliceAngle);
			const float cosSlice = glm::cos(sliceAngle);
			const glm::vec3 norm(sinSlice * sinStack, cosSlice * sinStack, cosStack);
			const glm::vec3 pos(norm * radius);
			addVertex(pos, glm::vec2(du * slice, dv * stack));
		}
	}

	// north-pole triangles
	const int startVertexIndex = 0;
	int rowA = startVertexIndex;
	int rowB = rowA + numSlices + 1;
	for (int slice = 0; slice < numSlices; slice++) {
		_indices.push_back(rowA + slice);
		_indices.push_back(rowB + slice);
		_indices.push_back(rowB + slice + 1);
	}

	// stack triangles
	for (int stack = 1; stack < numStacks - 1; stack++) {
		rowA = startVertexIndex + stack * (numSlices + 1);
		rowB = rowA + numSlices + 1;
		for (int slice = 0; slice < numSlices; slice++) {
			_indices.push_back(rowA + slice);
			_indices.push_back(rowB + slice + 1);
			_indices.push_back(rowA + slice + 1);

			_indices.push_back(rowA + slice);
			_indices.push_back(rowB + slice);
			_indices.push_back(rowB + slice + 1);
		}
	}

	// south-pole triangles
	rowA = startVertexIndex + (numStacks - 1) * (numSlices + 1);
	rowB = rowA + numSlices + 1;
	for (int slice = 0; slice < numSlices; slice++) {
		_indices.push_back(rowA + slice);
		_indices.push_back(rowB + slice + 1);
		_indices.push_back(rowA + slice + 1);
	}
}

void ShapeBuilder::shutdown() {
	clear();
}

}
