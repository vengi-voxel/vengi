/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "math/Tri.h"
#include "math/AABB.h"
#include "math/OBB.h"
#include "math/Plane.h"
#include "video/Types.h"
#include "core/collection/Buffer.h"
#include <glm/mat3x3.hpp>
#include <stdint.h>

namespace video {

class Camera;

enum class ShapeBuilderCube {
	None = 0,
	Front = 1 << 0,
	Right = 1 << 1,
	Back = 1 << 2,
	Left = 1 << 3,
	Bottom = 1 << 5,
	Top = 1 << 6,
	All = Front | Right | Back | Left | Bottom | Top
};
CORE_ENUM_BIT_OPERATIONS(ShapeBuilderCube);

/**
 * @brief Generates primitives and allows to generate geometry from other types like @c AABB,
 * @c Frustum and so on
 * @ingroup Video
 */
class ShapeBuilder {
public:
	typedef core::Buffer<uint32_t, 256> Indices;
	typedef core::Buffer<glm::vec3, 256> Vertices;
	typedef core::Buffer<glm::vec2, 256> Texcoords;
	typedef core::Buffer<glm::vec4, 256> Colors;
private:
	alignas(32) Indices _indices;
	alignas(32) Texcoords _texcoords;
	alignas(32) Vertices _vertices;
	alignas(32) Vertices _normals;
	alignas(32) Colors _colors;
	glm::mat3 _rotation = glm::mat3(1.0f);
	Primitive _primitive = Primitive::Triangles;
	int _initialSize;
	glm::vec4 _color;
	glm::vec3 _position {0.0f};
	bool _applyRotation = false;
public:
	ShapeBuilder(int initialSize = 0);

	/**
	 * @brief Reserve additional vertex data
	 *
	 * @param vertices The amount of additional vertices
	 * @param indices The amount of additional indices
	 */
	inline void reserve(int vertices, int indices) {
		_colors.reserve(_colors.size() + vertices);
		_vertices.reserve(_vertices.size() + vertices);
		_normals.reserve(_normals.size() + vertices);
		_texcoords.reserve(_texcoords.size() + vertices);

		_indices.reserve(_indices.size() + indices);
	}

	inline void addIndex(uint32_t index) {
		core_assert(_indices.capacity() > _indices.size());
		_indices.push_back(index);
	}

	inline void addIndex(uint32_t index1, uint32_t index2) {
		core_assert(_indices.capacity() >= _indices.size() + 2);
		core_assert(_primitive == Primitive::Lines || _primitive == Primitive::LineStrip ||
					_primitive == Primitive::LinesAdjacency);
		_indices.push_back(index1);
		_indices.push_back(index2);
	}

	inline void addIndex(uint32_t index1, uint32_t index2, uint32_t index3) {
		core_assert(_indices.capacity() >= _indices.size() + 3);
		_indices.push_back(index1);
		_indices.push_back(index2);
		_indices.push_back(index3);
	}

	void setPrimitive(Primitive primitive);

	inline Primitive primitive() const {
		return _primitive;
	}

	uint32_t addVertex(const glm::vec3& vertex, const glm::vec2& uv, const glm::vec3& normal = glm::vec3(0.0f));
	uint32_t addVertex(const glm::vec3& vertex, const glm::vec3& normal = glm::vec3(0.0f));
	void addTri(const math::Tri &tri, bool calcNormal = false);

	inline void clear() {
		_colors.clear();
		_vertices.clear();
		_indices.clear();
		_texcoords.clear();
		_normals.clear();
		_position = glm::vec3(0.0f);
		_rotation = glm::mat3(1.0f);
		_applyRotation = false;
		if (_initialSize > 0) {
			reserve(_initialSize, _initialSize);
		}
	}
	void aabbGridXY(const math::AABB<float>& aabb, bool near = false, float stepWidth = 1.0f, float thickness = 1.0f);
	void aabbGridYZ(const math::AABB<float>& aabb, bool near = false, float stepWidth = 1.0f, float thickness = 1.0f);
	void aabbGridXZ(const math::AABB<float>& aabb, bool near = false, float stepWidth = 1.0f, float thickness = 1.0f);

	void arrow(const glm::vec3 &point1, const glm::vec3 &point2, const glm::vec3 &point3, float thickness = 1.0f);

	void cylinder(float radius, float length, int slices = 20);

	/**
	 * @brief Creates a triangulated cone.
	 */
	void cone(float baseRadius, float length, int slices = 20);

	void cube(const glm::vec3& mins, const glm::vec3& maxs, ShapeBuilderCube sides = ShapeBuilderCube::All);
	/**
	 * @brief The bone is rendered into the positive z direction
	 */
	void bone(float length, float posSize = 1.0f, float boneSize = 2.0f);
	/**
	 * @brief The bone is rendered into the positive z direction
	 */
	void bone(const glm::vec3 &from, const glm::vec3 &to, float posSize = 1.0f, float boneSize = 2.0f);
	void diamond(float length1, float length2);

	void line(const glm::vec3& start, const glm::vec3& end, float thickness = 1.0f);

	void aabb(const glm::vec3& mins, const glm::vec3& maxs, float thickness = 1.0f);
	void obb(const math::OBBF& obb);
	void aabb(const math::AABB<float>& aabb, bool renderGrid = false, float stepWidth = 1.0f, float thickness = 1.0f);
	void aabb(const math::AABB<int>& aabb, bool renderGrid = false, float stepWidth = 1.0f, float thickness = 1.0f);
	/**
	 * @param[in] tesselation The amount of splits on the plane that should be made
	 */
	void plane(uint32_t tesselation = 10);
	void frustum(const Camera& camera, int splitFrustum = 0);
	void camera(const Camera& camera);
	void geom(const glm::vec3* vert, size_t vertCount, const uint32_t* indices, size_t indicesCount, Primitive primitive = Primitive::Triangles);
	void geom(const core::Buffer<glm::vec3>& vert, const core::Buffer<uint32_t>& indices, Primitive primitive = Primitive::Triangles);
	void plane(const math::Plane& plane, bool normal);
	void pyramid(const glm::vec3& size = glm::vec3(1.0f));
	/**
	 * Geometry layout for spheres is as follows (for 5 slices, 4 stacks):
	 *
	 * +  +  +  +  +  +        north pole
	 * |\ |\ |\ |\ |\
	 * | \| \| \| \| \
	 * +--+--+--+--+--+        30 vertices (slices + 1) * (stacks + 1)
	 * |\ |\ |\ |\ |\ |        30 triangles (2 * slices * stacks) - (2 * slices)
	 * | \| \| \| \| \|        2 orphan vertices, but f*ck it
	 * +--+--+--+--+--+
	 * |\ |\ |\ |\ |\ |
	 * | \| \| \| \| \|
	 * +--+--+--+--+--+
	 *  \ |\ |\ |\ |\ |
	 *   \| \| \| \| \|
	 * +  +  +  +  +  +        south pole
	 */
	void sphere(int numSlices, int numStacks, float radius);
	void axis(float scale) { axis(glm::vec3(scale)); }
	void axis(const glm::vec3& scale);
	/**
	 * @brief Frees the memory
	 */
	void shutdown();

	/**
	 * @brief The vertices that were generated in the @c init() method.
	 *
	 * @note They are normalized between -0.5 and 0.5 and their winding is counter clock wise
	 */
	const Vertices& getVertices() const;
	const Vertices& getNormals() const;
	void convertVertices(core::Buffer<glm::vec4>& out) const;

	template<class FUNC>
	size_t iterate(FUNC&& func) const {
		const size_t size = _vertices.size();
		for (size_t i = 0; i < size; ++i) {
			const glm::vec3& pos = _vertices[i];
			const glm::vec2& uv = _texcoords.empty() ? glm::vec2(0.0f) : _texcoords[i];
			const glm::vec4& color = _colors[i];
			const glm::vec3& normal = _normals[i];
			func(pos, uv, color, normal);
		}
		return size;
	}
	const Indices& getIndices() const;
	const Colors& getColors() const;
	const Texcoords& getTexcoords() const;

	bool setColor(const glm::vec4& color);
	void setPosition(const glm::vec3& position);
	void setRotation(const glm::mat3& rotation);
	void resetRotation();
};

inline void ShapeBuilder::resetRotation() {
	_applyRotation = false;
}

inline void ShapeBuilder::aabb(const math::AABB<int>& aabb, bool renderGrid, float stepWidth, float thickness) {
	const math::AABB<float> converted(glm::vec3(aabb.getLowerCorner()), glm::vec3(aabb.getUpperCorner()));
	this->aabb(converted, renderGrid, stepWidth, thickness);
}

inline void ShapeBuilder::setPosition(const glm::vec3& position) {
	_position = position;
}

inline void ShapeBuilder::setRotation(const glm::mat3& rotation) {
	_rotation = rotation;
	_applyRotation = true;
}

inline const ShapeBuilder::Vertices& ShapeBuilder::getVertices() const {
	return _vertices;
}

inline const ShapeBuilder::Vertices& ShapeBuilder::getNormals() const {
	return _normals;
}

inline void ShapeBuilder::convertVertices(core::Buffer<glm::vec4>& out) const {
	const ShapeBuilder::Vertices& vertices = getVertices();
	out.reserve(vertices.size());
	for (const ShapeBuilder::Vertices::value_type& v : vertices) {
		out.emplace_back(v.x, v.y, v.z, 1.0f);
	}
}

inline const ShapeBuilder::Indices& ShapeBuilder::getIndices() const {
	return _indices;
}

inline const ShapeBuilder::Colors& ShapeBuilder::getColors() const {
	return _colors;
}

inline const ShapeBuilder::Texcoords& ShapeBuilder::getTexcoords() const {
	return _texcoords;
}

}
