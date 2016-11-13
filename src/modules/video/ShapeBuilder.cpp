#include "ShapeBuilder.h"

namespace video {

void ShapeBuilder::aabbGridXY(const core::AABB<float>& aabb, bool near, float stepWidth) {
	setPrimitive(Primitive::Lines);
	const glm::vec3& width = aabb.getWidth();
	const glm::vec3& halfWidth = width / 2.0f;
	const glm::vec3& center = aabb.getCenter();
	const float wx = halfWidth.x + center.x;
	const float wy = halfWidth.y + center.y;
	const float wz = near ? 0.0f : center.z + halfWidth.z;
	for (float x = 0.0f; x <= width.x; x += stepWidth) {
		addIndex(addVertex(glm::vec3(x, 0.0f, wz), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
		addIndex(addVertex(glm::vec3(x, wy, wz), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
	}
	for (float y = 0.0f; y <= width.y; y += stepWidth) {
		addIndex(addVertex(glm::vec3(0.0f, y, wz), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
		addIndex(addVertex(glm::vec3(wx, y, wz), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
	}
}

void ShapeBuilder::aabbGridYZ(const core::AABB<float>& aabb, bool near, float stepWidth) {
	setPrimitive(Primitive::Lines);
	const glm::vec3& width = aabb.getWidth();
	const glm::vec3& halfWidth = width / 2.0f;
	const glm::vec3& center = aabb.getCenter();
	const float wx = near ? 0.0f : center.x + halfWidth.x;
	const float wy = halfWidth.y + center.y;
	const float wz = halfWidth.z + center.z;
	for (float y = 0.0f; y <= width.y; y += stepWidth) {
		addIndex(addVertex(glm::vec3(wx, y, 0.0f), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
		addIndex(addVertex(glm::vec3(wx, y, wz), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
	}
	for (float z = 0.0f; z <= width.z; z += stepWidth) {
		addIndex(addVertex(glm::vec3(wx, 0.0f, z), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
		addIndex(addVertex(glm::vec3(wx, wy, z), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
	}
}

void ShapeBuilder::aabbGridXZ(const core::AABB<float>& aabb, bool near, float stepWidth) {
	setPrimitive(Primitive::Lines);
	const glm::vec3& width = aabb.getWidth();
	const glm::vec3& halfWidth = width / 2.0f;
	const glm::vec3& center = aabb.getCenter();
	const float wx = halfWidth.x + center.x;
	const float wy = near ? 0.0f : center.y + halfWidth.y;
	const float wz = halfWidth.z + center.z;
	for (float x = 0.0f; x <= width.x; x += stepWidth) {
		addIndex(addVertex(glm::vec3(x, wy, 0.0f), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
		addIndex(addVertex(glm::vec3(x, wy, wz), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
	}
	for (float z = 0.0f; z <= width.z; z += stepWidth) {
		addIndex(addVertex(glm::vec3(0.0f, wy, z), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
		addIndex(addVertex(glm::vec3(wx, wy, z), glm::zero<glm::vec2>(), glm::zero<glm::vec3>()));
	}
}

void ShapeBuilder::aabb(const core::AABB<float>& aabb, bool renderGrid, float stepWidth) {
	setPrimitive(Primitive::Lines);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();
	static const glm::vec3 vecs[8] = {
		glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3(-1.0f, -1.0f,  1.0f),
		glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec3( 1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec3( 1.0f, -1.0f, -1.0f)
	};
	reserve(SDL_arraysize(vecs));
	const glm::vec3& width = aabb.getWidth();
	const glm::vec3& halfWidth = width / 2.0f;
	const glm::vec3& center = aabb.getCenter();
	for (size_t i = 0; i < SDL_arraysize(vecs); ++i) {
		addVertex(vecs[i] * halfWidth + center, glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
	}

	// front
	addIndex(startIndex + 0);
	addIndex(startIndex + 1);

	addIndex(startIndex + 1);
	addIndex(startIndex + 3);

	addIndex(startIndex + 3);
	addIndex(startIndex + 2);

	addIndex(startIndex + 2);
	addIndex(startIndex + 0);

	// back
	addIndex(startIndex + 4);
	addIndex(startIndex + 5);

	addIndex(startIndex + 5);
	addIndex(startIndex + 7);

	addIndex(startIndex + 7);
	addIndex(startIndex + 6);

	addIndex(startIndex + 6);
	addIndex(startIndex + 4);

	// connections
	addIndex(startIndex + 0);
	addIndex(startIndex + 4);

	addIndex(startIndex + 2);
	addIndex(startIndex + 6);

	addIndex(startIndex + 1);
	addIndex(startIndex + 5);

	addIndex(startIndex + 3);
	addIndex(startIndex + 7);

	if (renderGrid) {
		aabbGridXY(aabb, false, stepWidth);
		aabbGridXZ(aabb, false, stepWidth);
		aabbGridYZ(aabb, false, stepWidth);

		aabbGridXY(aabb, true, stepWidth);
		aabbGridXZ(aabb, true, stepWidth);
		aabbGridYZ(aabb, true, stepWidth);
	}
}

void ShapeBuilder::plane(const core::Plane& plane, bool normals) {
	setPrimitive(Primitive::Lines);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();
	const glm::vec3& planeNormal = plane.norm();
	const float planeScale = plane.dist();

	const glm::vec3& right = glm::cross(planeNormal, glm::up);
	const glm::vec3& up = glm::cross(right, planeNormal);
	const glm::mat4 rot(
		right.x, up.x, -planeNormal.x, 0.0f,
		right.y, up.y, -planeNormal.y, 0.0f,
		right.z, up.z, -planeNormal.z, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	const glm::mat4& trans = glm::translate(planeNormal * planeScale);
	const glm::mat4& result = trans * rot;

	static const glm::vec4 corners[] = {
		glm::vec4(-planeScale, -planeScale, 0.0f, 1.0f),
		glm::vec4(-planeScale,  planeScale, 0.0f, 1.0f),
		glm::vec4( planeScale,  planeScale, 0.0f, 1.0f),
		glm::vec4( planeScale, -planeScale, 0.0f, 1.0f)
	};

	setColor(core::Color::Green);
	for (uint32_t i = 0; i < SDL_arraysize(corners); ++i) {
		const glm::vec4& v = result * corners[i];
		addVertex(v.xyz(), glm::zero<glm::vec2>(), planeNormal);
	}

	const float normalVecScale = 10.0f;
	const glm::vec3& pvn = planeNormal * normalVecScale;
	setColor(core::Color::Red);
	addVertex(glm::zero<glm::vec3>(), glm::zero<glm::vec2>(), planeNormal);
	addVertex(pvn, glm::zero<glm::vec2>(), planeNormal);

	addIndex(startIndex + 0);
	addIndex(startIndex + 1);

	addIndex(startIndex + 1);
	addIndex(startIndex + 3);

	addIndex(startIndex + 0);
	addIndex(startIndex + 2);

	addIndex(startIndex + 3);
	addIndex(startIndex + 0);

	addIndex(startIndex + 3);
	addIndex(startIndex + 1);

	addIndex(startIndex + 1);
	addIndex(startIndex + 2);

	addIndex(startIndex + 2);
	addIndex(startIndex + 3);

	addIndex(startIndex + 4);
	addIndex(startIndex + 5);
}

void ShapeBuilder::frustum(const Camera& camera, int splitFrustum) {
	setPrimitive(Primitive::Lines);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();
	glm::vec3 out[core::FRUSTUM_VERTICES_MAX];
	uint32_t indices[core::FRUSTUM_VERTICES_MAX * 3];
	camera.frustumCorners(out, indices);

	const int targetLineVertices = camera.rotationType() == CameraRotationType::Target ? 2 : 0;

	if (splitFrustum > 0) {
		int indexOffset = startIndex;
		float planes[splitFrustum * 2];

		camera.sliceFrustum(planes, SDL_arraysize(planes), splitFrustum);

		reserve(core::FRUSTUM_VERTICES_MAX * splitFrustum + targetLineVertices);

		for (int s = 0; s < splitFrustum; ++s) {
			const float near = planes[s * 2 + 0];
			const float far = planes[s * 2 + 1];
			camera.splitFrustum(near, far, out);

			for (size_t i = 0; i < SDL_arraysize(out); ++i) {
				addVertex(out[i], glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
			}

			for (size_t i = 0; i < SDL_arraysize(indices); ++i) {
				addIndex(indexOffset + indices[i]);
			}
			indexOffset += core::FRUSTUM_VERTICES_MAX;
		}
	} else {
		reserve(core::FRUSTUM_VERTICES_MAX + targetLineVertices);

		for (size_t i = 0; i < SDL_arraysize(out); ++i) {
			addVertex(out[i], glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
		}

		for (size_t i = 0; i < SDL_arraysize(indices); ++i) {
			addIndex(startIndex + indices[i]);
		}
	}

	if (camera.rotationType() == CameraRotationType::Target) {
		setColor(core::Color::Green);
		addVertex(camera.position(), glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
		addVertex(camera.target(), glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
		addIndex(startIndex + core::FRUSTUM_VERTICES_MAX + 0);
		addIndex(startIndex + core::FRUSTUM_VERTICES_MAX + 1);
	}
}

void ShapeBuilder::axis(float scale) {
	setPrimitive(Primitive::Lines);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();
	const glm::vec3 verticesAxis[] = {
			 glm::vec3( 0.0f,   0.0f,   0.0f),
			 glm::vec3(scale,   0.0f,   0.0f),
			 glm::vec3( 0.0f,   0.0f,   0.0f),
			 glm::vec3( 0.0f,  scale,   0.0f),
			 glm::vec3( 0.0f,   0.0f,   0.0f),
			 glm::vec3( 0.0f,   0.0f,  scale)};

	setColor(core::Color::Red);
	addVertex(verticesAxis[0], glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
	addVertex(verticesAxis[1], glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
	setColor(core::Color::Green);
	addVertex(verticesAxis[2], glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
	addVertex(verticesAxis[3], glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
	setColor(core::Color::Blue);
	addVertex(verticesAxis[4], glm::zero<glm::vec2>(), glm::zero<glm::vec3>());
	addVertex(verticesAxis[5], glm::zero<glm::vec2>(), glm::zero<glm::vec3>());

	for (size_t i = 0; i < SDL_arraysize(verticesAxis); ++i) {
		addIndex(startIndex + i);
	}
}

void ShapeBuilder::plane(uint32_t tesselation, float scale) {
	setPrimitive(Primitive::Triangles);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();
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
			addVertex(v * scale, uv, glm::zero<glm::vec3>());
		}
	}

	for (size_t y = 0; y < tesselation + 1; ++y) {
		for (size_t x = 0; x < tesselation + 1; ++x) {
			addIndex(startIndex + (y * strucWidth) + x);
			addIndex(startIndex + (y * strucWidth) + x + 1);
			addIndex(startIndex + ((y + 1) * strucWidth) + x);
			addIndex(startIndex + ((y + 1) * strucWidth) + x);
			addIndex(startIndex + (y * strucWidth) + x + 1);
			addIndex(startIndex + ((y + 1) * strucWidth) + x + 1);
		}
	}
}

void ShapeBuilder::sphere(int numSlices, int numStacks, float radius) {
	setPrimitive(Primitive::Triangles);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();
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
			addVertex(pos, glm::vec2(du * slice, dv * stack), norm);
		}
	}

	// north-pole triangles
	int rowA = startIndex;
	int rowB = rowA + numSlices + 1;
	for (int slice = 0; slice < numSlices; slice++) {
		addIndex(rowA + slice);
		addIndex(rowB + slice);
		addIndex(rowB + slice + 1);
	}

	// stack triangles
	for (int stack = 1; stack < numStacks - 1; stack++) {
		rowA = startIndex + stack * (numSlices + 1);
		rowB = rowA + numSlices + 1;
		for (int slice = 0; slice < numSlices; slice++) {
			addIndex(rowA + slice);
			addIndex(rowB + slice + 1);
			addIndex(rowA + slice + 1);

			addIndex(rowA + slice);
			addIndex(rowB + slice);
			addIndex(rowB + slice + 1);
		}
	}

	// south-pole triangles
	rowA = startIndex + (numStacks - 1) * (numSlices + 1);
	rowB = rowA + numSlices + 1;
	for (int slice = 0; slice < numSlices; slice++) {
		addIndex(rowA + slice);
		addIndex(rowB + slice + 1);
		addIndex(rowA + slice + 1);
	}
}

void ShapeBuilder::shutdown() {
	clear();
}

}
