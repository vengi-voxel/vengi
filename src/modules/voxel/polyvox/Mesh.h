/**
 * @file
 */

#pragma once

#include "Region.h"
#include "VoxelVertex.h"
#include <algorithm>
#include <stdlib.h>
#include <memory>
#include <vector>

namespace voxel {

// TODO: maybe reduce to uint16_t and use glDrawElementsBaseVertex
typedef uint32_t IndexType;

/**
 * @brief A simple and general-purpose mesh class to represent the data returned by the surface extraction functions.
 *
 * @note You are only able to store vertex ranges from 0 to 255 here, due to the limited data type of the position in
 * the Vertex class.
 */
class Mesh {
public:
	Mesh(int vertices, int indices, bool mayGetResized = false);

	/**
	 * @brief Calculate the memory amount this mesh is using
	 */
	size_t size();

	/**
	 * @brief Transforms another mesh into this mesh class. The indices are offset.
	 * @return @c true if the given mesh is compatible to this mesh instance, @c false
	 * otherwise.
	 * @note Incompatible mesh means that the offsets don't match. This is needed due to the limited range of vertices
	 * due to the Vertex class position data type. Therefore we can merge meshes, but only if the offset is the same
	 * (as we can't exceed the 0-255 range).
	 */
	bool addMesh(const Mesh& mesh);

	size_t getNoOfVertices() const;
	const VoxelVertex& getVertex(IndexType index) const;
	const VoxelVertex* getRawVertexData() const;

	size_t getNoOfIndices() const;
	IndexType getIndex(IndexType index) const;
	const IndexType* getRawIndexData() const;

	const std::vector<IndexType>& getIndexVector() const;
	const std::vector<VoxelVertex>& getVertexVector() const;
	std::vector<IndexType>& getIndexVector();
	std::vector<VoxelVertex>& getVertexVector();

	const glm::ivec3& getOffset() const;
	void setOffset(const glm::ivec3& offset);

	IndexType addVertex(const VoxelVertex& vertex);
	void addTriangle(IndexType index0, IndexType index1, IndexType index2);

	void clear();
	bool isEmpty() const;
	void removeUnusedVertices();

private:
	alignas(16) std::vector<IndexType> _vecIndices;
	alignas(16) std::vector<VoxelVertex> _vecVertices;
	glm::ivec3 _offset = glm::zero<glm::ivec3>();
	bool _mayGetResized;
};

inline Mesh::Mesh(int vertices, int indices, bool mayGetResized) : _mayGetResized(mayGetResized) {
	if (vertices > 0) {
		_vecVertices.reserve(vertices);
	}
	if (indices > 0) {
		_vecIndices.reserve(indices);
	}
}

inline const std::vector<IndexType>& Mesh::getIndexVector() const {
	return _vecIndices;
}

inline const std::vector<VoxelVertex>& Mesh::getVertexVector() const {
	return _vecVertices;
}

inline std::vector<IndexType>& Mesh::getIndexVector() {
	return _vecIndices;
}

inline std::vector<VoxelVertex>& Mesh::getVertexVector() {
	return _vecVertices;
}

inline size_t Mesh::getNoOfVertices() const {
	return _vecVertices.size();
}

inline const VoxelVertex& Mesh::getVertex(IndexType index) const {
	return _vecVertices[index];
}

inline const VoxelVertex* Mesh::getRawVertexData() const {
	return _vecVertices.data();
}

inline size_t Mesh::getNoOfIndices() const {
	return _vecIndices.size();
}

inline IndexType Mesh::getIndex(IndexType index) const {
	return _vecIndices[index];
}

inline const IndexType* Mesh::getRawIndexData() const {
	return _vecIndices.data();
}

inline const glm::ivec3& Mesh::getOffset() const {
	return _offset;
}

inline void Mesh::setOffset(const glm::ivec3& offset) {
	_offset = offset;
}

inline void Mesh::addTriangle(IndexType index0, IndexType index1, IndexType index2) {
	//Make sure the specified indices correspond to valid vertices.
	core_assert_msg(index0 < _vecVertices.size(), "Index points at an invalid vertex.");
	core_assert_msg(index1 < _vecVertices.size(), "Index points at an invalid vertex.");
	core_assert_msg(index2 < _vecVertices.size(), "Index points at an invalid vertex.");
	if (!_mayGetResized) {
		core_assert_msg(_vecIndices.size() + 3 < _vecIndices.capacity(), "addTriangle() call exceeds the capacity of the indices vector and will trigger a realloc (%i vs %i)", (int)_vecIndices.size(), (int)_vecIndices.capacity());
	}

	_vecIndices.push_back(index0);
	_vecIndices.push_back(index1);
	_vecIndices.push_back(index2);
}

inline IndexType Mesh::addVertex(const VoxelVertex& vertex) {
	// We should not add more vertices than our chosen index type will let us index.
	core_assert_msg(_vecVertices.size() < (std::numeric_limits<IndexType>::max)(), "Mesh has more vertices that the chosen index type allows.");
	if (!_mayGetResized) {
		core_assert_msg(_vecVertices.size() + 1 < _vecVertices.capacity(), "addVertex() call exceeds the capacity of the vertices vector and will trigger a realloc (%i vs %i)", (int)_vecVertices.size(), (int)_vecVertices.capacity());
	}

	_vecVertices.push_back(vertex);
	return _vecVertices.size() - 1;
}

inline void Mesh::clear() {
	_vecVertices.clear();
	_vecIndices.clear();
	_offset = glm::zero<glm::ivec3>();
}

inline bool Mesh::isEmpty() const {
	return getNoOfVertices() == 0 || getNoOfIndices() == 0;
}

}
