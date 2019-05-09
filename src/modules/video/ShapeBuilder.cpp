/**
 * @file
 */

#include "ShapeBuilder.h"
#include "math/Frustum.h"

namespace video {

ShapeBuilder::ShapeBuilder(int initialSize) :
		_initialSize(initialSize) {
	if (_initialSize > 0) {
		reserve(initialSize);
	}
}

void ShapeBuilder::aabbGridXY(const math::AABB<float>& aabb, bool near, float stepWidth) {
	setPrimitive(Primitive::Lines);
	const glm::vec3& mins = aabb.mins();
	const glm::vec3& width = aabb.getWidth();
	const float wz = near ? 0.0f : width.z;
	for (float x = 0.0f; x <= width.x; x += stepWidth) {
		addIndex(addVertex(glm::vec3(x, 0.0f, wz) + mins));
		addIndex(addVertex(glm::vec3(x, width.y, wz) + mins));
	}
	for (float y = 0.0f; y <= width.y; y += stepWidth) {
		addIndex(addVertex(glm::vec3(0.0f, y, wz) + mins));
		addIndex(addVertex(glm::vec3(width.x, y, wz) + mins));
	}
}

void ShapeBuilder::aabbGridYZ(const math::AABB<float>& aabb, bool near, float stepWidth) {
	setPrimitive(Primitive::Lines);
	const glm::vec3& mins = aabb.mins();
	const glm::vec3& width = aabb.getWidth();
	const float wx = near ? 0.0f : width.x;
	for (float y = 0.0f; y <= width.y; y += stepWidth) {
		addIndex(addVertex(glm::vec3(wx, y, 0.0f) + mins));
		addIndex(addVertex(glm::vec3(wx, y, width.z) + mins));
	}
	for (float z = 0.0f; z <= width.z; z += stepWidth) {
		addIndex(addVertex(glm::vec3(wx, 0.0f, z) + mins));
		addIndex(addVertex(glm::vec3(wx, width.y, z) + mins));
	}
}

void ShapeBuilder::aabbGridXZ(const math::AABB<float>& aabb, bool near, float stepWidth) {
	setPrimitive(Primitive::Lines);
	const glm::vec3& mins = aabb.mins();
	const glm::vec3& width = aabb.getWidth();
	const float wy = near ? 0.0f : width.y;
	for (float x = 0.0f; x <= width.x; x += stepWidth) {
		addIndex(addVertex(glm::vec3(x, wy, 0.0f) + mins));
		addIndex(addVertex(glm::vec3(x, wy, width.z) + mins));
	}
	for (float z = 0.0f; z <= width.z; z += stepWidth) {
		addIndex(addVertex(glm::vec3(0.0f, wy, z) + mins));
		addIndex(addVertex(glm::vec3(width.x, wy, z) + mins));
	}
}

void ShapeBuilder::line(const glm::vec3& start, const glm::vec3& end) {
	setPrimitive(Primitive::Lines);
	reserve(2);
	addIndex(addVertex(start));
	addIndex(addVertex(end));
}

void ShapeBuilder::cube(const glm::vec3& mins, const glm::vec3& maxs) {
	setPrimitive(Primitive::Triangles);

	// indices
	const uint32_t startIndex = (uint32_t)_vertices.size();

	reserve(8, 12);

	// front
	addVertex(glm::vec3(mins.x, mins.y, maxs.z));
	addVertex(glm::vec3(maxs.x, mins.y, maxs.z));
	addVertex(glm::vec3(maxs.x, maxs.y, maxs.z));
	addVertex(glm::vec3(mins.x, maxs.y, maxs.z));
	// back
	addVertex(glm::vec3(mins.x, mins.y, mins.z));
	addVertex(glm::vec3(maxs.x, mins.y, mins.z));
	addVertex(glm::vec3(maxs.x, maxs.y, mins.z));
	addVertex(glm::vec3(mins.x, maxs.y, mins.z));

	// front
	addIndex(startIndex + 0, startIndex + 1, startIndex + 2);
	addIndex(startIndex + 2, startIndex + 3, startIndex + 0);
	// right
	addIndex(startIndex + 1, startIndex + 5, startIndex + 6);
	addIndex(startIndex + 6, startIndex + 2, startIndex + 1);
	// back
	addIndex(startIndex + 7, startIndex + 6, startIndex + 5);
	addIndex(startIndex + 5, startIndex + 4, startIndex + 7);
	// left
	addIndex(startIndex + 4, startIndex + 0, startIndex + 3);
	addIndex(startIndex + 3, startIndex + 7, startIndex + 4);
	// bottom
	addIndex(startIndex + 4, startIndex + 5, startIndex + 1);
	addIndex(startIndex + 1, startIndex + 0, startIndex + 4);
	// top
	addIndex(startIndex + 3, startIndex + 2, startIndex + 6);
	addIndex(startIndex + 6, startIndex + 7, startIndex + 3);
}

void ShapeBuilder::aabb(const math::AABB<float>& aabb, bool renderGrid, float stepWidth) {
	setPrimitive(Primitive::Lines);
	const uint32_t startIndex = (uint32_t)_vertices.size();
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
		addVertex(vecs[i] * halfWidth + center);
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

void ShapeBuilder::geom(const std::vector<glm::vec3>& vert, const std::vector<uint32_t>& indices, Primitive primitive) {
	setPrimitive(primitive);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();

	reserve(vert.size());

	for (const glm::vec3& v : vert) {
		addVertex(v);
	}

	for (uint32_t i : indices) {
		addIndex(startIndex + i);
	}
}

void ShapeBuilder::plane(const math::Plane& plane, bool normals) {
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

	reserve(SDL_arraysize(corners) + 2);

	setColor(core::Color::Green);
	for (uint32_t i = 0; i < SDL_arraysize(corners); ++i) {
		const glm::vec4& v = result * corners[i];
		addVertex(glm::vec3(v), planeNormal);
	}

	if (normals) {
		const float normalVecScale = 10.0f;
		const glm::vec3& pvn = planeNormal * normalVecScale;
		setColor(core::Color::Red);
		addVertex(glm::zero<glm::vec3>(), planeNormal);
		addVertex(pvn, planeNormal);
	}

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

	if (normals) {
		addIndex(startIndex + 4);
		addIndex(startIndex + 5);
	}
}

void ShapeBuilder::pyramid(const glm::vec3& size) {
	setPrimitive(Primitive::Triangles);

	reserve(12);

	const glm::vec3& tip = glm::vec3(_position.x, _position.y + size.y, _position.z);
	const glm::vec3& vlfl = glm::vec3(_position.x - size.x, _position.y, _position.z + size.z);
	const glm::vec3& vlfr = glm::vec3(_position.x + size.x, _position.y, _position.z + size.z);
	const glm::vec3& vlbl = glm::vec3(_position.x - size.x, _position.y, _position.z - size.z);
	const glm::vec3& vlbr = glm::vec3(_position.x + size.x, _position.y, _position.z - size.z);
	addIndex(addVertex(tip));
	addIndex(addVertex(vlfl));
	addIndex(addVertex(vlfr));

	addIndex(addVertex(tip));
	addIndex(addVertex(vlbl));
	addIndex(addVertex(vlbr));

	addIndex(addVertex(tip));
	addIndex(addVertex(vlbl));
	addIndex(addVertex(vlfl));

	addIndex(addVertex(tip));
	addIndex(addVertex(vlbr));
	addIndex(addVertex(vlfr));
}

void ShapeBuilder::frustum(const Camera& camera, int splitFrustum) {
	setPrimitive(Primitive::Lines);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();
	glm::vec3 out[math::FRUSTUM_VERTICES_MAX];
	uint32_t indices[math::FRUSTUM_VERTICES_MAX * 3];
	camera.frustumCorners(out, indices);

	const int targetLineVertices = camera.rotationType() == CameraRotationType::Target ? 2 : 0;

	if (splitFrustum > 0) {
		int indexOffset = startIndex;
		std::vector<float> planes;
		planes.reserve(splitFrustum * 2);

		camera.sliceFrustum(&planes[0], splitFrustum * 2, splitFrustum);

		reserve(math::FRUSTUM_VERTICES_MAX * splitFrustum + targetLineVertices);

		for (int splitStep = 0; splitStep < splitFrustum; ++splitStep) {
			const float near = planes[splitStep * 2 + 0];
			const float far = planes[splitStep * 2 + 1];
			camera.splitFrustum(near, far, out);

			for (size_t i = 0; i < SDL_arraysize(out); ++i) {
				addVertex(out[i]);
			}

			for (size_t i = 0; i < SDL_arraysize(indices); ++i) {
				addIndex(indexOffset + indices[i]);
			}
			indexOffset += math::FRUSTUM_VERTICES_MAX;
		}
	} else {
		reserve(math::FRUSTUM_VERTICES_MAX + targetLineVertices);

		for (size_t i = 0; i < SDL_arraysize(out); ++i) {
			addVertex(out[i]);
		}

		for (size_t i = 0; i < SDL_arraysize(indices); ++i) {
			addIndex(startIndex + indices[i]);
		}
	}

	if (camera.rotationType() == CameraRotationType::Target) {
		setColor(core::Color::Green);
		addVertex(camera.position());
		addVertex(camera.target());
		addIndex(startIndex + math::FRUSTUM_VERTICES_MAX + 0);
		addIndex(startIndex + math::FRUSTUM_VERTICES_MAX + 1);
	}
}

void ShapeBuilder::axis(const glm::vec3& scale) {
	setPrimitive(Primitive::Lines);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();
	const glm::vec3 verticesAxis[] = {
			 glm::vec3(_position.x,           _position.y,           _position.z),
			 glm::vec3(_position.x + scale.x, _position.y,           _position.z),
			 glm::vec3(_position.x,           _position.y,           _position.z),
			 glm::vec3(_position.x,           _position.y + scale.y, _position.z),
			 glm::vec3(_position.x,           _position.y,           _position.z),
			 glm::vec3(_position.x,           _position.y,           _position.z + scale.z)};

	reserve(SDL_arraysize(verticesAxis));

	setColor(core::Color::Red);
	addVertex(verticesAxis[0]);
	addVertex(verticesAxis[1]);
	setColor(core::Color::Green);
	addVertex(verticesAxis[2]);
	addVertex(verticesAxis[3]);
	setColor(core::Color::Blue);
	addVertex(verticesAxis[4]);
	addVertex(verticesAxis[5]);

	for (size_t i = 0; i < SDL_arraysize(verticesAxis); ++i) {
		addIndex(startIndex + i);
	}
}

void ShapeBuilder::plane(uint32_t tesselation) {
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
			addVertex(v, uv, glm::zero<glm::vec3>());
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
	const float du = 1.0f / numSlices;
	const float dv = 1.0f / numStacks;

	reserve(numStacks * numSlices);

	for (int stack = 0; stack <= numStacks; stack++) {
		const float stackAngle = (glm::pi<float>() * stack) / numStacks;
		const float sinStack = glm::sin(stackAngle);
		const float cosStack = glm::cos(stackAngle);
		for (int slice = 0; slice <= numSlices; slice++) {
			const float sliceAngle = (glm::two_pi<float>() * slice) / numSlices;
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

size_t ShapeBuilder::iterate(std::function<void(const glm::vec3& pos, const glm::vec2& uv, const glm::vec4& color, const glm::vec3& normal)> func) const {
	const size_t size = _vertices.size();
	for (size_t i = 0; i < size; ++i) {
		const glm::vec3& pos = _vertices[i];
		const glm::vec2& uv = _texcoords.empty() ? glm::zero<glm::vec2>() : _texcoords[i];
		const glm::vec4& color = _colors[i];
		const glm::vec3& normal = _normals[i];
		func(pos, uv, color, normal);
	}
	return size;
}

void ShapeBuilder::shutdown() {
	clear();
}

}
