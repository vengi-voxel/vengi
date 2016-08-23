#pragma once

#include "core/Common.h"
#include "core/AABB.h"
#include "video/Camera.h"
#include "core/Color.h"
#include <stdint.h>
#include <vector>

namespace video {

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
	Colors _colors;

	glm::vec4 _color = core::Color::Red;
	glm::vec3 _position;

	inline void reserve(int vertices, int additionalIndices = 0) {
		_colors.reserve(_colors.size() + vertices + additionalIndices);
		_vertices.reserve(_vertices.size() + vertices);
		_indices.reserve(_indices.size() + vertices * 3 + additionalIndices);
		_texcoords.reserve(_texcoords.size() + vertices);
	}

	inline void addVertex(const glm::vec3& vertex, const glm::vec2& uv) {
		_colors.push_back(_color);
		_vertices.push_back(_position + vertex);
		_texcoords.push_back(uv);
	}
public:
	inline void clear() {
		_colors.clear();
		_vertices.clear();
		_indices.clear();
		_texcoords.clear();
		_position = glm::vec3();
	}

	void aabb(const core::AABB<float>& aabb);
	/**
	 * @param[in] tesselation The amount of splits on the plane that should be made
	 */
	void plane(uint32_t tesselation = 10, float scale = 1.0f);
	void frustum(const Camera& camera);
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
	void axis(float scale);
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
	void convertVertices(std::vector<glm::vec4>& out) const;
	const Indices& getIndices() const;
	const Colors& getColors() const;
	const Texcoords& getTexcoords() const;

	void setColor(const glm::vec4& color);
	void setPosition(const glm::vec3& position);
};

inline void ShapeBuilder::setColor(const glm::vec4& color) {
	_color = color;
}

inline void ShapeBuilder::setPosition(const glm::vec3& position) {
	_position = position;
}

inline const ShapeBuilder::Vertices& ShapeBuilder::getVertices() const {
	return _vertices;
}

inline void ShapeBuilder::convertVertices(std::vector<glm::vec4>& out) const {
	const ShapeBuilder::Vertices& vertices = getVertices();
	out.reserve(vertices.size());
	for (const ShapeBuilder::Vertices::value_type& v : vertices) {
		out.emplace_back(v, 1.0f);
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
