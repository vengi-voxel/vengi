/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "math/AABB.h"
#include "math/Plane.h"
#include "video/Camera.h"
#include "video/Types.h"
#include "core/GLM.h"
#include "core/Color.h"
#include <stdint.h>
#include <vector>

namespace video {

/**
 * @brief Generates primitives and allows to generate geometry from other types like @c AABB,
 * @c Frustum and so on
 * @ingroup Video
 */
class ShapeBuilder {
public:
	typedef std::vector<uint32_t> Indices;
	typedef Indices::const_iterator IndicesIter;
	typedef std::vector<glm::vec3> Vertices;
	typedef Vertices::const_iterator VerticesIter;
	typedef std::vector<glm::vec2> Texcoords;
	typedef Texcoords::const_iterator TexcoordsIter;
	typedef std::vector<glm::vec4> Colors;
	typedef Colors::const_iterator ColorsIter;
private:
	Indices _indices;
	Texcoords _texcoords;
	Vertices _vertices;
	Vertices _normals;
	Colors _colors;
	glm::mat3 _rotation = glm::mat3(1.0f);
	Primitive _primitive = Primitive::Triangles;
	int _initialSize;
	glm::vec4 _color = core::Color::White;
	glm::vec3 _position {0.0f};
public:
	ShapeBuilder(int initialSize = 0);

	inline void reserve(int vertices, int additionalIndices = 0) {
		_colors.reserve(_colors.size() + vertices + additionalIndices);
		_vertices.reserve(_vertices.size() + vertices);
		_normals.reserve(_normals.size() + vertices);
		_indices.reserve(_indices.size() + vertices * 3 + additionalIndices);
		_texcoords.reserve(_texcoords.size() + vertices);
	}

	inline void addIndex(uint32_t index) {
		_indices.push_back(index);
	}

	inline void addIndex(uint32_t index1, uint32_t index2, uint32_t index3) {
		_indices.push_back(index1);
		_indices.push_back(index2);
		_indices.push_back(index3);
	}

	inline void setPrimitive(Primitive primitive) {
		core_assert(_vertices.empty() || primitive == _primitive);
		_primitive = primitive;
	}

	inline Primitive primitive() const {
		return _primitive;
	}

	uint32_t addVertex(const glm::vec3& vertex, const glm::vec2& uv, const glm::vec3& normal = glm::zero<glm::vec3>());
	uint32_t addVertex(const glm::vec3& vertex, const glm::vec3& normal = glm::zero<glm::vec3>());

	inline void clear() {
		_colors.clear();
		_vertices.clear();
		_indices.clear();
		_texcoords.clear();
		_position = glm::zero<glm::vec3>();
		if (_initialSize > 0) {
			reserve(_initialSize);
		}
	}
	void aabbGridXY(const math::AABB<float>& aabb, bool near = false, float stepWidth = 1.0f);
	void aabbGridYZ(const math::AABB<float>& aabb, bool near = false, float stepWidth = 1.0f);
	void aabbGridXZ(const math::AABB<float>& aabb, bool near = false, float stepWidth = 1.0f);

	inline void cylinder(float radius, float length) {
		if (radius <= 0.0f) {
			return;
		}
		if (length <= 0.0f) {
			return;
		}
		cone(radius, 0.0f, length, -1, false, true);
	}

	/**
	 * @brief Creates a triangulated cone.
	 * @param[in] topRadius how many verts the top circle should have - if this is 0 it's a top cone
	 * @param[in] bottomRadius how many verts the bottom circle should have - if this is 0 it's a bottom cone
	 * @param[in] length how many world units should the cone be in length
	 * @param[in] openingAngle override the top and bottom for an angle of opening instead
	 * @param[in] inside the mesh should have inside geometry
	 * @param[in] outside the mesh should have outside geometry
	 */
	void cone(float topRadius, float bottomRadius, float length, int openingAngle = -1, bool inside = false, bool outside = true);

	void cube(const glm::vec3& mins, const glm::vec3& maxs);

	void line(const glm::vec3& start, const glm::vec3& end, float thickness = 1.0f);

	void aabb(const glm::vec3& mins, const glm::vec3& maxs);
	void aabb(const math::AABB<float>& aabb, bool renderGrid = false, float stepWidth = 1.0f);
	void aabb(const math::AABB<int>& aabb, bool renderGrid = false, float stepWidth = 1.0f);
	/**
	 * @param[in] tesselation The amount of splits on the plane that should be made
	 */
	void plane(uint32_t tesselation = 10);
	void frustum(const Camera& camera, int splitFrustum = 0);
	void geom(const std::vector<glm::vec3>& vert, const std::vector<uint32_t>& indices, Primitive primitive = Primitive::Triangles);
	void plane(const math::Plane& plane, bool normal);
	void pyramid(const glm::vec3& size = glm::one<glm::vec3>());
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
	void convertVertices(std::vector<glm::vec4>& out) const;
	size_t iterate(std::function<void(const glm::vec3& pos, const glm::vec2& uv, const glm::vec4& color, const glm::vec3& normal)> func) const;
	const Indices& getIndices() const;
	const Colors& getColors() const;
	const Texcoords& getTexcoords() const;

	void setColor(const glm::vec4& color);
	void setPosition(const glm::vec3& position);
	void setRotation(const glm::mat3& rotation);
};

inline void ShapeBuilder::aabb(const math::AABB<int>& aabb, bool renderGrid, float stepWidth) {
	const math::AABB<float> converted(glm::vec3(aabb.getLowerCorner()), glm::vec3(aabb.getUpperCorner()));
	this->aabb(converted, renderGrid, stepWidth);
}

inline void ShapeBuilder::setColor(const glm::vec4& color) {
	_color = color;
}

inline void ShapeBuilder::setPosition(const glm::vec3& position) {
	_position = position;
}

inline void ShapeBuilder::setRotation(const glm::mat3& rotation) {
	_rotation = rotation;
}

inline const ShapeBuilder::Vertices& ShapeBuilder::getVertices() const {
	return _vertices;
}

inline const ShapeBuilder::Vertices& ShapeBuilder::getNormals() const {
	return _normals;
}

inline void ShapeBuilder::convertVertices(std::vector<glm::vec4>& out) const {
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
