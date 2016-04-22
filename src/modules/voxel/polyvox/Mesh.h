#pragma once

#include "Region.h"
#include "Vertex.h" //Should probably do away with this one in the future...
#include <algorithm>
#include <cstdlib>
#include <list>
#include <memory>
#include <set>
#include <vector>

namespace voxel {

/// A simple and general-purpose mesh class to represent the data returned by the surface extraction functions.
/// It supports different vertex types (which will vary depending on the surface extractor used and the contents
/// of the volume) and both 16-bit and 32 bit indices.
typedef uint32_t DefaultIndexType;
template<typename _VertexType, typename _IndexType = DefaultIndexType>
class Mesh {
public:

	typedef _VertexType VertexType;
	typedef _IndexType IndexType;

	Mesh();
	~Mesh();

	IndexType getNoOfVertices() const;
	const VertexType& getVertex(IndexType index) const;
	const VertexType* getRawVertexData() const;

	size_t getNoOfIndices() const;
	IndexType getIndex(uint32_t index) const;
	const IndexType* getRawIndexData() const;

	const glm::ivec3& getOffset() const;
	void setOffset(const glm::ivec3& offset);

	IndexType addVertex(const VertexType& vertex);
	void addTriangle(IndexType index0, IndexType index1, IndexType index2);

	void clear();
	bool isEmpty() const;
	void removeUnusedVertices();

private:
	std::vector<IndexType> m_vecIndices;
	std::vector<VertexType> m_vecVertices;
	glm::ivec3 m_offset;
};

/// Meshes returned by the surface extractors often have vertices with efficient compressed
/// formats which are hard to interpret directly (see CubicVertex and MarchingCubesVertex).
/// This function creates a new uncompressed mesh containing the much simpler Vertex objects.
Mesh<Vertex> decodeMesh(const Mesh<CubicVertex>& encodedMesh);

template<typename VertexType, typename IndexType>
Mesh<VertexType, IndexType>::Mesh() {
}

template<typename VertexType, typename IndexType>
Mesh<VertexType, IndexType>::~Mesh() {
}

template<typename VertexType, typename IndexType>
IndexType Mesh<VertexType, IndexType>::getNoOfVertices() const {
	return static_cast<IndexType>(m_vecVertices.size());
}

template<typename VertexType, typename IndexType>
const VertexType& Mesh<VertexType, IndexType>::getVertex(IndexType index) const {
	return m_vecVertices[index];
}

template<typename VertexType, typename IndexType>
const VertexType* Mesh<VertexType, IndexType>::getRawVertexData() const {
	return m_vecVertices.data();
}

template<typename VertexType, typename IndexType>
size_t Mesh<VertexType, IndexType>::getNoOfIndices() const {
	return m_vecIndices.size();
}

template<typename VertexType, typename IndexType>
IndexType Mesh<VertexType, IndexType>::getIndex(uint32_t index) const {
	return m_vecIndices[index];
}

template<typename VertexType, typename IndexType>
const IndexType* Mesh<VertexType, IndexType>::getRawIndexData() const {
	return m_vecIndices.data();
}

template<typename VertexType, typename IndexType>
const glm::ivec3& Mesh<VertexType, IndexType>::getOffset() const {
	return m_offset;
}

template<typename VertexType, typename IndexType>
void Mesh<VertexType, IndexType>::setOffset(const glm::ivec3& offset) {
	m_offset = offset;
}

template<typename VertexType, typename IndexType>
void Mesh<VertexType, IndexType>::addTriangle(IndexType index0, IndexType index1, IndexType index2) {
	//Make sure the specified indices correspond to valid vertices.
	core_assert_msg(index0 < m_vecVertices.size(), "Index points at an invalid vertex.");
	core_assert_msg(index1 < m_vecVertices.size(), "Index points at an invalid vertex.");
	core_assert_msg(index2 < m_vecVertices.size(), "Index points at an invalid vertex.");

	m_vecIndices.push_back(index0);
	m_vecIndices.push_back(index1);
	m_vecIndices.push_back(index2);
}

template<typename VertexType, typename IndexType>
IndexType Mesh<VertexType, IndexType>::addVertex(const VertexType& vertex) {
	// We should not add more vertices than our chosen index type will let us index.
	core_assert_msg(m_vecVertices.size() < std::numeric_limits<IndexType>::max(), "Mesh has more vertices that the chosen index type allows.");

	m_vecVertices.push_back(vertex);
	return m_vecVertices.size() - 1;
}

template<typename VertexType, typename IndexType>
void Mesh<VertexType, IndexType>::clear() {
	m_vecVertices.clear();
	m_vecIndices.clear();
}

template<typename VertexType, typename IndexType>
bool Mesh<VertexType, IndexType>::isEmpty() const {
	return (getNoOfVertices() == 0) || (getNoOfIndices() == 0);
}

template<typename VertexType, typename IndexType>
void Mesh<VertexType, IndexType>::removeUnusedVertices() {
	std::vector<bool> isVertexUsed(m_vecVertices.size());
	std::fill(isVertexUsed.begin(), isVertexUsed.end(), false);

	for (uint32_t triCt = 0; triCt < m_vecIndices.size(); triCt++) {
		int v = m_vecIndices[triCt];
		isVertexUsed[v] = true;
	}

	int noOfUsedVertices = 0;
	std::vector<uint32_t> newPos(m_vecVertices.size());
	for (IndexType vertCt = 0; vertCt < m_vecVertices.size(); vertCt++) {
		if (isVertexUsed[vertCt]) {
			m_vecVertices[noOfUsedVertices] = m_vecVertices[vertCt];
			newPos[vertCt] = noOfUsedVertices;
			noOfUsedVertices++;
		}
	}

	m_vecVertices.resize(noOfUsedVertices);

	for (uint32_t triCt = 0; triCt < m_vecIndices.size(); triCt++) {
		m_vecIndices[triCt] = newPos[m_vecIndices[triCt]];
	}
}

}
