/**
 * @file
 */

#include "ShapeBuilder.h"
#include "Camera.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/ArrayLength.h"
#include "math/Frustum.h"
#include "color/ColorUtil.h"
#include "core/GLM.h"
#include "core/ArrayLength.h"
#include "video/Types.h"
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/vector_relational.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

#undef near
#undef far

namespace video {

ShapeBuilder::ShapeBuilder(int initialSize) :
		_initialSize(initialSize), _color(color::White()) {
	if (_initialSize > 0) {
		reserve(initialSize, initialSize);
	}
}

void ShapeBuilder::setPrimitive(Primitive primitive) {
	core_assert(_vertices.empty() || primitive == _primitive);
	_primitive = primitive;
}

void ShapeBuilder::aabbGridXY(const math::AABB<float>& aabb, bool near, float stepWidth, float thickness) {
	const glm::vec3& mins = aabb.mins();
	const glm::vec3& width = aabb.getWidth();
	const float wz = near ? 0.0f : width.z;
	const int n = (int)(width.y / stepWidth) + (int)(width.z / stepWidth) + 2;
	const glm::vec4 darkerColor = color::darker(_color);
	const glm::vec4 color = _color;
	reserve(n * 2, n * 2);
	int i = 0;
	for (float x = 0.0f; x <= width.x; x += stepWidth, ++i) {
		setColor(i % 5 == 0 ? color : darkerColor);
		line(glm::vec3(x, 0.0f, wz) + mins, glm::vec3(x, width.y, wz) + mins, thickness);
	}
	i = 0;
	for (float y = 0.0f; y <= width.y; y += stepWidth, ++i) {
		setColor(i % 5 == 0 ? color : darkerColor);
		line(glm::vec3(0.0f, y, wz) + mins, glm::vec3(width.x, y, wz) + mins, thickness);
	}
	setColor(color);
}

void ShapeBuilder::aabbGridYZ(const math::AABB<float>& aabb, bool near, float stepWidth, float thickness) {
	const glm::vec3& mins = aabb.mins();
	const glm::vec3& width = aabb.getWidth();
	const float wx = near ? 0.0f : width.x;
	const int n = (int)(width.y / stepWidth) + (int)(width.z / stepWidth) + 2;
	const glm::vec4 darkerColor = color::darker(_color);
	const glm::vec4 color = _color;
	reserve(n * 2, n * 2);
	int i = 0;
	for (float y = 0.0f; y <= width.y; y += stepWidth, ++i) {
		setColor(i % 5 == 0 ? color : darkerColor);
		line(glm::vec3(wx, y, 0.0f) + mins, glm::vec3(wx, y, width.z) + mins, thickness);
	}
	i = 0;
	for (float z = 0.0f; z <= width.z; z += stepWidth, ++i) {
		setColor(i % 5 == 0 ? color : darkerColor);
		line(glm::vec3(wx, 0.0f, z) + mins, glm::vec3(wx, width.y, z) + mins, thickness);
	}
	setColor(color);
}

void ShapeBuilder::aabbGridXZ(const math::AABB<float>& aabb, bool near, float stepWidth, float thickness) {
	const glm::vec3& mins = aabb.mins();
	const glm::vec3& width = aabb.getWidth();
	const float wy = near ? 0.0f : width.y;
	const int n = (int)(width.y / stepWidth) + (int)(width.z / stepWidth) + 2;
	const glm::vec4 darkerColor = color::darker(_color);
	const glm::vec4 color = _color;
	reserve(n * 2, n * 2);
	int i = 0;
	for (float x = 0.0f; x <= width.x; x += stepWidth, ++i) {
		setColor(i % 5 == 0 ? color : darkerColor);
		line(glm::vec3(x, wy, 0.0f) + mins, glm::vec3(x, wy, width.z) + mins, thickness);
	}
	i = 0;
	for (float z = 0.0f; z <= width.z; z += stepWidth, ++i) {
		setColor(i % 5 == 0 ? color : darkerColor);
		line(glm::vec3(0.0f, wy, z) + mins, glm::vec3(width.x, wy, z) + mins, thickness);
	}
	setColor(color);
}

void ShapeBuilder::arrow(const glm::vec3 &left, const glm::vec3 &center, const glm::vec3 &right, float thickness) {
	line(left, center, thickness);
	line(center, right, thickness);
}

bool ShapeBuilder::setColor(const glm::vec4& color) {
	if (glm::all(glm::epsilonEqual(_color, color, glm::epsilon<float>()))) {
		return false;
	}
	_color = color;
	return true;
}

void ShapeBuilder::line(const glm::vec3& start, const glm::vec3& end, float thickness) {
	if (thickness <= 1.0f) {
		setPrimitive(Primitive::Lines);
		reserve(2, 2);
		addIndex(addVertex(start));
		addIndex(addVertex(end));
	} else {
		glm::vec3 d = end - start;
		const float d2 = glm::dot(d, d);
		if (d2 > glm::epsilon<float>()) {
			const float invLen = glm::inversesqrt(d2);
			d *= invLen;
		}
		d *= (thickness * 0.5f);

		// TODO: RENDERER: this is broken - cube is of course not correct
		const float dp = core_max(d.x, core_max(d.y, d.z));
		const glm::vec3 mins(start.x + dp, start.y - dp, start.z - dp);
		const glm::vec3 maxs(  end.x - dp,   end.y + dp,   end.z + dp);
		cube(mins, maxs);
	}
}

void ShapeBuilder::cube(const glm::vec3& mins, const glm::vec3& maxs, ShapeBuilderCube sides) {
	if (sides == ShapeBuilderCube::None) {
		return;
	}
	setPrimitive(Primitive::Triangles);

	// indices
	const uint32_t startIndex = (uint32_t)_vertices.size();

	reserve(8, 36);

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

	if ((sides & ShapeBuilderCube::Front) == ShapeBuilderCube::Front) {
		addIndex(startIndex + 0, startIndex + 1, startIndex + 2);
		addIndex(startIndex + 2, startIndex + 3, startIndex + 0);
	}
	if ((sides & ShapeBuilderCube::Right) == ShapeBuilderCube::Right) {
		addIndex(startIndex + 1, startIndex + 5, startIndex + 6);
		addIndex(startIndex + 6, startIndex + 2, startIndex + 1);
	}
	if ((sides & ShapeBuilderCube::Back) == ShapeBuilderCube::Back) {
		addIndex(startIndex + 7, startIndex + 6, startIndex + 5);
		addIndex(startIndex + 5, startIndex + 4, startIndex + 7);
	}
	if ((sides & ShapeBuilderCube::Left) == ShapeBuilderCube::Left) {
		addIndex(startIndex + 4, startIndex + 0, startIndex + 3);
		addIndex(startIndex + 3, startIndex + 7, startIndex + 4);
	}
	if ((sides & ShapeBuilderCube::Bottom) == ShapeBuilderCube::Bottom) {
		addIndex(startIndex + 4, startIndex + 5, startIndex + 1);
		addIndex(startIndex + 1, startIndex + 0, startIndex + 4);
	}
	if ((sides & ShapeBuilderCube::Top) == ShapeBuilderCube::Top) {
		addIndex(startIndex + 3, startIndex + 2, startIndex + 6);
		addIndex(startIndex + 6, startIndex + 7, startIndex + 3);
	}
}

void ShapeBuilder::obb(const math::OBBF& obb) {
	setPrimitive(Primitive::Lines);
	reserve(8, 24);

	const glm::vec3& center = obb.origin();
	const glm::mat3x3 rot(obb.rotation());
	int indices[8];
	if (rot != glm::mat3(1.0f)) {
		const glm::vec3& halfWidth = obb.extents();
		const glm::vec3 vecs[8] = {
			glm::vec3(-halfWidth.x,  halfWidth.y,  halfWidth.z), glm::vec3(-halfWidth.x, -halfWidth.y,  halfWidth.z),
			glm::vec3( halfWidth.x,  halfWidth.y,  halfWidth.z), glm::vec3( halfWidth.x, -halfWidth.y,  halfWidth.z),
			glm::vec3(-halfWidth.x,  halfWidth.y, -halfWidth.z), glm::vec3(-halfWidth.x, -halfWidth.y, -halfWidth.z),
			glm::vec3( halfWidth.x,  halfWidth.y, -halfWidth.z), glm::vec3( halfWidth.x, -halfWidth.y, -halfWidth.z)
		};
		for (size_t i = 0; i < lengthof(vecs); ++i) {
			indices[i] = addVertex(rot * vecs[i] + center);
		}
	} else {
		const glm::vec3& halfWidth = obb.extents();
		indices[0] = addVertex(glm::vec3(-halfWidth.x + center.x,  halfWidth.y + center.y,  halfWidth.z + center.z));
		indices[1] = addVertex(glm::vec3(-halfWidth.x + center.x, -halfWidth.y + center.y,  halfWidth.z + center.z));
		indices[2] = addVertex(glm::vec3( halfWidth.x + center.x,  halfWidth.y + center.y,  halfWidth.z + center.z));
		indices[3] = addVertex(glm::vec3( halfWidth.x + center.x, -halfWidth.y + center.y,  halfWidth.z + center.z));
		indices[4] = addVertex(glm::vec3(-halfWidth.x + center.x,  halfWidth.y + center.y, -halfWidth.z + center.z));
		indices[5] = addVertex(glm::vec3(-halfWidth.x + center.x, -halfWidth.y + center.y, -halfWidth.z + center.z));
		indices[6] = addVertex(glm::vec3( halfWidth.x + center.x,  halfWidth.y + center.y, -halfWidth.z + center.z));
		indices[7] = addVertex(glm::vec3( halfWidth.x + center.x, -halfWidth.y + center.y, -halfWidth.z + center.z));
	}

	// front
	addIndex(indices[0], indices[1]);
	addIndex(indices[1], indices[3]);
	addIndex(indices[3], indices[2]);
	addIndex(indices[2], indices[0]);

	// back
	addIndex(indices[4], indices[5]);
	addIndex(indices[5], indices[7]);
	addIndex(indices[7], indices[6]);
	addIndex(indices[6], indices[4]);

	// connections
	addIndex(indices[0], indices[4]);
	addIndex(indices[2], indices[6]);
	addIndex(indices[1], indices[5]);
	addIndex(indices[3], indices[7]);
}

void ShapeBuilder::aabb(const math::AABB<float>& aabb, bool renderGrid, float stepWidth, float thickness) {
	glm::vec3 vecs[8] = {
		glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3(-1.0f, -1.0f,  1.0f),
		glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec3( 1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec3( 1.0f, -1.0f, -1.0f)
	};
	const glm::vec3& width = aabb.getWidth();
	const glm::vec3& halfWidth = width / 2.0f;
	const glm::vec3& center = aabb.getCenter();

	for (size_t i = 0; i < lengthof(vecs); ++i) {
		vecs[i] = vecs[i] * halfWidth + center;
	}

	reserve(24, 24);

	// front
	line(vecs[0], vecs[1], thickness);
	line(vecs[1], vecs[3], thickness);
	line(vecs[3], vecs[2], thickness);
	line(vecs[2], vecs[0], thickness);

	// back
	line(vecs[4], vecs[5], thickness);
	line(vecs[5], vecs[7], thickness);
	line(vecs[7], vecs[6], thickness);
	line(vecs[6], vecs[4], thickness);

	// connections
	line(vecs[0], vecs[4], thickness);
	line(vecs[2], vecs[6], thickness);
	line(vecs[1], vecs[5], thickness);
	line(vecs[3], vecs[7], thickness);

	if (renderGrid) {
		aabbGridXY(aabb, false, stepWidth, thickness);
		aabbGridXZ(aabb, false, stepWidth, thickness);
		aabbGridYZ(aabb, false, stepWidth, thickness);

		aabbGridXY(aabb, true, stepWidth, thickness);
		aabbGridXZ(aabb, true, stepWidth, thickness);
		aabbGridYZ(aabb, true, stepWidth, thickness);
	}
}

void ShapeBuilder::aabb(const glm::vec3& mins, const glm::vec3& maxs, float thickness) {
	glm::vec3 vecs[8] = {
		glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3(-1.0f, -1.0f,  1.0f),
		glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec3( 1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(-1.0f, -1.0f, -1.0f),
		glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec3( 1.0f, -1.0f, -1.0f)
	};
	reserve(lengthof(vecs), 24);
	const glm::vec3& width = maxs - mins;
	const glm::vec3& halfWidth = width / 2.0f;
	const glm::vec3& center = maxs - halfWidth;
	for (size_t i = 0; i < lengthof(vecs); ++i) {
		vecs[i] = vecs[i] * halfWidth + center;
	}

	// front
	line(vecs[0], vecs[1], thickness);
	line(vecs[1], vecs[3], thickness);
	line(vecs[3], vecs[2], thickness);
	line(vecs[2], vecs[0], thickness);

	// back
	line(vecs[4], vecs[5], thickness);
	line(vecs[5], vecs[7], thickness);
	line(vecs[7], vecs[6], thickness);
	line(vecs[6], vecs[4], thickness);

	// connections
	line(vecs[0], vecs[4], thickness);
	line(vecs[2], vecs[6], thickness);
	line(vecs[1], vecs[5], thickness);
	line(vecs[3], vecs[7], thickness);
}

void ShapeBuilder::geom(const glm::vec3* vert, size_t vertCount, const uint32_t* indices, size_t indicesCount, Primitive primitive) {
	setPrimitive(primitive);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();

	reserve(vertCount, indicesCount);

	for (size_t i = 0; i < vertCount; ++i) {
		addVertex(vert[i]);
	}

	for (size_t i = 0; i < indicesCount; ++i) {
		addIndex(startIndex + indices[i]);
	}
}

void ShapeBuilder::geom(const core::Buffer<glm::vec3>& vert, const core::Buffer<uint32_t>& indices, Primitive primitive) {
	geom(&vert.front(), vert.size(), &indices.front(), indices.size(), primitive);
}

void ShapeBuilder::plane(const math::Plane& plane, bool normals) {
	setPrimitive(Primitive::Lines);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t)_vertices.size();
	const glm::vec3& planeNormal = plane.norm();
	const float planeScale = plane.dist();

	const glm::vec3& right = glm::cross(planeNormal, glm::up());
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

	reserve(lengthof(corners) + 2, 16);

	setColor(color::Green());
	for (uint32_t i = 0; i < lengthof(corners); ++i) {
		const glm::vec4& v = result * corners[i];
		addVertex(glm::vec3(v), planeNormal);
	}

	if (normals) {
		const float normalVecScale = 10.0f;
		const glm::vec3& pvn = planeNormal * normalVecScale;
		setColor(color::Red());
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

	reserve(5, 18);

	const glm::vec3& tip = glm::vec3(_position.x, _position.y + size.y, _position.z);
	const glm::vec3& vlfl = glm::vec3(_position.x - size.x, _position.y, _position.z + size.z);
	const glm::vec3& vlfr = glm::vec3(_position.x + size.x, _position.y, _position.z + size.z);
	const glm::vec3& vlbl = glm::vec3(_position.x - size.x, _position.y, _position.z - size.z);
	const glm::vec3& vlbr = glm::vec3(_position.x + size.x, _position.y, _position.z - size.z);
	const uint32_t startIndex = _vertices.empty() ? 0u : (uint32_t) _vertices.size();

	addVertex(tip);
	addVertex(vlfl);
	addVertex(vlfr);
	addVertex(vlbl);
	addVertex(vlbr);

	// back side
	addIndex(startIndex + 0);
	addIndex(startIndex + 1);
	addIndex(startIndex + 2);

	// front side
	addIndex(startIndex + 0);
	addIndex(startIndex + 3);
	addIndex(startIndex + 4);

	// left side
	addIndex(startIndex + 0);
	addIndex(startIndex + 3);
	addIndex(startIndex + 1);

	// right side
	addIndex(startIndex + 0);
	addIndex(startIndex + 4);
	addIndex(startIndex + 2);

	// bottom quad (two triangles)
	addIndex(startIndex + 1);
	addIndex(startIndex + 3);
	addIndex(startIndex + 2);
	addIndex(startIndex + 2);
	addIndex(startIndex + 3);
	addIndex(startIndex + 4);
}

uint32_t ShapeBuilder::addVertex(const glm::vec3& vertex, const glm::vec2& uv, const glm::vec3& normal) {
	core_assert(_colors.capacity() > _colors.size());
	_colors.push_back(_color);
	core_assert(_vertices.capacity() > _vertices.size());
	if (_applyRotation) {
		_vertices.push_back(_position + _rotation * vertex);
	} else {
		_vertices.push_back(_position + vertex);
	}
	core_assert(_normals.capacity() > _normals.size());
	_normals.push_back(normal);
	core_assert(_texcoords.capacity() > _texcoords.size());
	_texcoords.push_back(uv);
	core_assert(_texcoords.size() == _vertices.size());
	return (uint32_t)_vertices.size() - 1;
}

uint32_t ShapeBuilder::addVertex(const glm::vec3& vertex, const glm::vec3& normal) {
	core_assert(_colors.capacity() > _colors.size());
	_colors.push_back(_color);
	core_assert(_vertices.capacity() > _vertices.size());
	if (_applyRotation) {
		_vertices.push_back(_position + _rotation * vertex);
	} else {
		_vertices.push_back(_position + vertex);
	}
	core_assert(_normals.capacity() > _normals.size());
	_normals.push_back(normal);
	core_assert(_texcoords.empty());
	return (uint32_t)_vertices.size() - 1;
}

void ShapeBuilder::cylinder(float radius, float length, int slices) {
	if (radius <= 0.0f) {
		return;
	}
	if (length <= 0.0f) {
		return;
	}

	setPrimitive(Primitive::Triangles);

	reserve(slices + 2, (2 + slices) * 3 * 2);
	const float invNumVerts = 1.0f / float(slices);

	const uint32_t capTopIndex = addVertex(glm::zero<glm::vec3>());
	const uint32_t capBottomIndex = addVertex(glm::vec3(0.0f, 0.0f, length));
	const uint32_t startIndex = (uint32_t)_vertices.size();

	// the bottom sides of the cone
	for (int j = 0; j < slices; j++) {
		const float angle = glm::two_pi<float>() * (float)j * invNumVerts;
		const float angleSin = glm::sin(angle);
		const float angleCos = glm::cos(angle);
		addVertex(glm::vec3(radius * angleCos, radius * angleSin, length));
	}

	const uint32_t topIndexStart = (uint32_t)_vertices.size();

	// the top sides of the cone
	for (int j = 0; j < slices; j++) {
		const float angle = glm::two_pi<float>() * (float)j * invNumVerts;
		const float angleSin = glm::sin(angle);
		const float angleCos = glm::cos(angle);
		addVertex(glm::vec3(radius * angleCos, radius * angleSin, 0.0f));
	}

	for (int i = 0; i < slices; ++i) {
		int ip1 = i + 1;
		if (ip1 == slices) {
			ip1 = 0;
		}

		addIndex(startIndex + i + slices);
		addIndex(startIndex + ip1);
		addIndex(startIndex + i);

		addIndex(startIndex + ip1);
		addIndex(startIndex + i + slices);
		addIndex(startIndex + ip1 + slices);
	}

	// bottom indices
	for (int i = 0; i < slices; ++i) {
		addIndex(startIndex + i);
		if (i == slices - 1) {
			addIndex(startIndex);
		} else {
			addIndex(startIndex + i + 1);
		}
		addIndex(capBottomIndex);
	}

	// top indices
	for (int i = 0; i < slices; ++i) {
		if (i == slices - 1) {
			addIndex(topIndexStart);
		} else {
			addIndex(topIndexStart + i + 1);
		}
		addIndex(topIndexStart + i);
		addIndex(capTopIndex);
	}
}

void ShapeBuilder::diamond(float length1, float length2) {
	// pos diamond - tip of bone
	const float halfLength1 = length1 / 2.0f;

	if (_primitive == Primitive::Lines) {
		reserve(6, 24);

		const uint32_t diamond = addVertex(glm::vec3(0.0f, 0.0f, 0.0f));

		addVertex(glm::vec3(-halfLength1,  halfLength1, length1));
		addVertex(glm::vec3( halfLength1,  halfLength1, length1));
		addVertex(glm::vec3( halfLength1, -halfLength1, length1));
		addVertex(glm::vec3(-halfLength1, -halfLength1, length1));

		const uint32_t diamond2 = addVertex(glm::vec3(0.0f, 0.0f, length1 + length2));

		addIndex(diamond, diamond + 1);
		addIndex(diamond, diamond + 2);
		addIndex(diamond, diamond + 3);
		addIndex(diamond, diamond + 4);

		addIndex(diamond + 1, diamond + 2);
		addIndex(diamond + 2, diamond + 3);
		addIndex(diamond + 3, diamond + 4);
		addIndex(diamond + 4, diamond + 1);

		addIndex(diamond2, diamond + 1);
		addIndex(diamond2, diamond + 2);
		addIndex(diamond2, diamond + 3);
		addIndex(diamond2, diamond + 4);
	} else if (_primitive == Primitive::Triangles) {
		const glm::vec3 v0(0.0f, 0.0f, 0.0f);
		const glm::vec3 v1(-halfLength1,  halfLength1, length1);
		const glm::vec3 v2( halfLength1,  halfLength1, length1);
		const glm::vec3 v3( halfLength1, -halfLength1, length1);
		const glm::vec3 v4(-halfLength1, -halfLength1, length1);
		const glm::vec3 v5(0.0f, 0.0f, length1 + length2);

		math::Tri tris[8];
		tris[0].setColor(color::darker(_color, 2.0f));
		tris[0].setVertices(v0, v1, v2);

		tris[1].setColor(color::brighter(_color, 2.0f));
		tris[1].setVertices(v0, v2, v3);

		tris[2].setColor(_color);
		tris[2].setVertices(v0, v3, v4);

		tris[3].setColor(_color);
		tris[3].setVertices(v0, v4, v1);

		tris[4].setColor(color::darker(_color));
		tris[4].setVertices(v5, v2, v1);

		tris[5].setColor(color::brighter(_color));
		tris[5].setVertices(v5, v3, v2);

		tris[6].setColor(_color);
		tris[6].setVertices(v5, v4, v3);

		tris[7].setColor(_color);
		tris[7].setVertices(v5, v1, v4);

		reserve(lengthof(tris) * 3, lengthof(tris) * 3);
		for (int i = 0; i < lengthof(tris); ++i) {
			addTri(tris[i], true);
		}
	}
}

void ShapeBuilder::addTri(const math::Tri &tri, bool calcNormal) {
	const glm::vec3 &n = calcNormal ? glm::normalize(tri.normal()) : glm::zero<glm::vec3>();
	glm::vec4 copy = _color;
	setColor(color::fromRGBA(tri.color0()));
	addIndex(addVertex(tri.vertex0(), n));
	setColor(color::fromRGBA(tri.color1()));
	addIndex(addVertex(tri.vertex1(), n));
	setColor(color::fromRGBA(tri.color2()));
	addIndex(addVertex(tri.vertex2(), n));
	setColor(copy);
}

void ShapeBuilder::bone(const glm::vec3 &from, const glm::vec3 &to, float posSize, float boneSize) {
	// backup state
	const glm::vec3 prevPos = _position;
	const glm::mat4 prevRotation = _rotation;
	const bool prevApplyRotation = _applyRotation;

	glm::vec3 dir = to - from;
	dir.z = -dir.z;
	const glm::vec3 norm = glm::normalize(dir);
	const float length = glm::distance(from, to);

	// change state
	setRotation(glm::mat4_cast(glm::rotation(norm, glm::forward())));
	setPosition(from);
	bone(length, posSize, boneSize);

	// restore state with old values
	_position = prevPos;
	_rotation = prevRotation;
	_applyRotation = prevApplyRotation;
}

void ShapeBuilder::bone(float length, float posSize, float boneSize) {
	if (_primitive == Primitive::Lines) {
		reserve(6 * 3, 24 * 3);
	} else if (_primitive == Primitive::Triangles) {
		reserve(8 * 3 * 3, 8 * 3 * 3);
	}

	const glm::vec3 pos = _position;
	diamond(posSize, posSize);
	if (_applyRotation) {
		_position += _rotation * glm::vec3(0.0f, 0.0f, 2.0f * posSize);
	} else {
		_position.z += 2.0f * posSize;
	}
	length -= (4.0f * posSize + boneSize);
	if (length > 0.0f) {
		diamond(boneSize, length);
		if (_applyRotation) {
			_position += _rotation * glm::vec3(0.0f, 0.0f, boneSize + length);
		} else {
			_position.z += boneSize + length;
		}
	}
	diamond(posSize, posSize);
	_position = pos;
}

void ShapeBuilder::cone(float baseRadius, float length, int slices) {
	if (baseRadius <= 0.0f) {
		return;
	}
	if (length <= 0.0f) {
		return;
	}

	setPrimitive(Primitive::Triangles);

	reserve(slices + 2, (2 + slices) * 3 * 2);

	const uint32_t tipConeIndex = addVertex(glm::zero<glm::vec3>());
	const uint32_t capCenterIndex = addVertex(glm::vec3(0.0f, 0.0f, length));
	const uint32_t startIndex = (uint32_t)_vertices.size();

	// the sides of the cone
	const float invNumVerts = 1.0f / float(slices);
	for (int j = 0; j < slices; j++) {
		const float angle = glm::two_pi<float>() * (float)j * invNumVerts;
		const float angleSin = glm::sin(angle);
		const float angleCos = glm::cos(angle);
		addVertex(glm::vec3(baseRadius * angleCos, baseRadius * angleSin, length));
	}

	for (int i = 0; i < slices; ++i) {
		if (i == slices - 1) {
			addIndex(startIndex);
		} else {
			addIndex(startIndex + i + 1);
		}
		addIndex(startIndex + i);
		addIndex(tipConeIndex);
	}

	for (int i = 0; i < slices; ++i) {
		addIndex(startIndex + i);
		if (i == slices - 1) {
			addIndex(startIndex);
		} else {
			addIndex(startIndex + i + 1);
		}
		addIndex(capCenterIndex);
	}
}

void ShapeBuilder::frustum(const Camera& camera, int splitFrustum) {
	setPrimitive(Primitive::Lines);
	glm::vec3 out[math::FRUSTUM_VERTICES_MAX];
	uint32_t indices[math::FRUSTUM_VERTICES_MAX * 3];
	camera.frustumCorners(out, indices);

	if (splitFrustum > 0) {
		core::Buffer<float> planes(splitFrustum * 2);
		camera.sliceFrustum(&planes[0], splitFrustum * 2, splitFrustum);

		for (int splitStep = 0; splitStep < splitFrustum; ++splitStep) {
			const float near = planes[splitStep * 2 + 0];
			const float far = planes[splitStep * 2 + 1];
			camera.splitFrustum(near, far, out);

			for (size_t i = 0; i < lengthof(indices); i += 2) {
				line(out[indices[i]], out[indices[i + 1]]);
			}
		}
	} else {
		for (size_t i = 0; i < lengthof(indices); i += 2) {
			line(out[indices[i]], out[indices[i + 1]]);
		}
	}

	if (camera.rotationType() == CameraRotationType::Target) {
		setColor(color::Green());
		line(camera.worldPosition(), camera.target());
	}
}

void ShapeBuilder::axis(const glm::vec3& scale) {
	setColor(color::Red());
	line(glm::zero<glm::vec3>(), glm::right() * scale);

	setColor(color::Green());
	line(glm::zero<glm::vec3>(), glm::up() * scale);

	setColor(color::Blue());
	line(glm::zero<glm::vec3>(), glm::forward() * scale);
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
	const float segmentWidth = 1.0f / (float)(tesselation + 1);
	const float scaleX = meshBounds.x / (float)(tesselation + 1);
	const float scaleY = meshBounds.y / (float)(tesselation + 1);

	reserve(strucWidth * strucWidth, (tesselation + 1) * (tesselation + 1) * 6);

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
	const float du = 1.0f / (float)numSlices;
	const float dv = 1.0f / (float)numStacks;

	const int nindices = numSlices * 2 * 3 + numSlices * 6 * numStacks;
	reserve(numStacks * numSlices, nindices);

	for (int stack = 0; stack <= numStacks; stack++) {
		const float stackAngle = (glm::pi<float>() * (float)stack) / (float)numStacks;
		const float sinStack = glm::sin(stackAngle);
		const float cosStack = glm::cos(stackAngle);
		for (int slice = 0; slice <= numSlices; slice++) {
			const float sliceAngle = (glm::two_pi<float>() * (float)slice) / (float)numSlices;
			const float sinSlice = glm::sin(sliceAngle);
			const float cosSlice = glm::cos(sliceAngle);
			const glm::vec3 norm(sinSlice * sinStack, cosSlice * sinStack, cosStack);
			const glm::vec3 pos(norm * radius);
			if (_vertices.size() == _texcoords.size()) {
				addVertex(pos, glm::vec2(du * (float)slice, dv * (float)stack), norm);
			} else {
				addVertex(pos, norm);
			}
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
