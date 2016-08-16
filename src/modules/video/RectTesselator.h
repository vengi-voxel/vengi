#pragma once

#include "core/Common.h"
#include <stdint.h>
#include <vector>

namespace video {

class RectTesselator {
public:
	typedef std::vector<uint32_t> Indices;
	typedef Indices::const_iterator IndicesIter;
	typedef std::vector<glm::vec2> Vertices;
	typedef Vertices::const_iterator VerticesIter;
	typedef std::vector<glm::vec2> Texcoords;
	typedef Texcoords::const_iterator TexcoordsIter;
private:
	Indices _indices;
	Texcoords _texcoords;
	Vertices _vertices;
public:
	/**
	 * @param[in] tesselation The amount of splits on the plane that should be made
	 */
	void init(uint32_t tesselation = 10);
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
	const Indices& getIndices() const;
	const Texcoords& getTexcoords() const;
};

inline const RectTesselator::Vertices& RectTesselator::getVertices() const {
	return _vertices;
}

inline const RectTesselator::Indices& RectTesselator::getIndices() const {
	return _indices;
}

inline const RectTesselator::Texcoords& RectTesselator::getTexcoords() const {
	return _texcoords;
}

}
