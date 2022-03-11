/**
 * @file
 */

#pragma once

#include "VoxelVertex.h"
#include "core/collection/DynamicArray.h"

namespace voxel {

using VertexArray = core::DynamicArray<voxel::VoxelVertex>;
using IndexArray = core::DynamicArray<voxel::IndexType>;

/**
 * @brief A simple and general-purpose mesh class to represent the data returned by the surface extraction functions.
 */
class Mesh {
public:
	Mesh(int vertices, int indices, bool mayGetResized = false);
	Mesh() : Mesh(128, 128, true) {}
	Mesh(Mesh&& other) noexcept;
	Mesh(const Mesh& other);
	~Mesh();

	Mesh& operator=(const Mesh& other);
	Mesh& operator=(Mesh&& other) noexcept;

	size_t getNoOfVertices() const;
	const VoxelVertex& getVertex(IndexType index) const;
	const VoxelVertex* getRawVertexData() const;

	size_t getNoOfIndices() const;
	IndexType getIndex(IndexType index) const;
	const IndexType* getRawIndexData() const;

	const IndexArray& getIndexVector() const;
	const VertexArray& getVertexVector() const;
	IndexArray& getIndexVector();
	VertexArray& getVertexVector();

	const glm::ivec3& getOffset() const;
	void setOffset(const glm::ivec3& offset);

	IndexType addVertex(const VoxelVertex& vertex);
	void addTriangle(IndexType index0, IndexType index1, IndexType index2);

	void clear();
	bool isEmpty() const;
	void removeUnusedVertices();
	void compressIndices();

	const uint8_t* compressedIndices() const;
	size_t compressedIndexSize() const;

	bool operator<(const Mesh& rhs) const;
private:
	alignas(16) IndexArray _vecIndices;
	alignas(16) VertexArray _vecVertices;
	uint8_t *_compressedIndices = nullptr;
	size_t _compressedIndexSize = 0u;
	glm::ivec3 _offset { 0 };
	bool _mayGetResized;
};

inline const uint8_t* Mesh::compressedIndices() const {
	return _compressedIndices;
}

inline size_t Mesh::compressedIndexSize() const {
	return _compressedIndexSize;
}

}
