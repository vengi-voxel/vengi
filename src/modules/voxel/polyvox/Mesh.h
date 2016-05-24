/**
 * @file
 */

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

// TODO: maybe reduce to uint16_t and use glDrawElementsBaseVertex
typedef uint16_t IndexType;

/**
 * A simple and general-purpose mesh class to represent the data returned by the surface extraction functions.
 * It supports different vertex types (which will vary depending on the surface extractor used and the contents
 * of the volume).
 */
template<typename VertexType>
class Mesh {
public:
	Mesh();
	Mesh(int vertices);
	~Mesh();

	size_t getNoOfVertices() const;
	const VertexType& getVertex(IndexType index) const;
	const VertexType* getRawVertexData() const;

	size_t getNoOfIndices() const;
	IndexType getIndex(IndexType index) const;
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

template<typename VertexType>
Mesh<VertexType>::Mesh(int vertices) {
	m_vecVertices.reserve(vertices);
}

template<typename VertexType>
Mesh<VertexType>::Mesh() {
}

template<typename VertexType>
Mesh<VertexType>::~Mesh() {
}

template<typename VertexType>
size_t Mesh<VertexType>::getNoOfVertices() const {
	return m_vecVertices.size();
}

template<typename VertexType>
const VertexType& Mesh<VertexType>::getVertex(IndexType index) const {
	return m_vecVertices[index];
}

template<typename VertexType>
const VertexType* Mesh<VertexType>::getRawVertexData() const {
	return m_vecVertices.data();
}

template<typename VertexType>
size_t Mesh<VertexType>::getNoOfIndices() const {
	return m_vecIndices.size();
}

template<typename VertexType>
IndexType Mesh<VertexType>::getIndex(IndexType index) const {
	return m_vecIndices[index];
}

template<typename VertexType>
const IndexType* Mesh<VertexType>::getRawIndexData() const {
	return m_vecIndices.data();
}

template<typename VertexType>
const glm::ivec3& Mesh<VertexType>::getOffset() const {
	return m_offset;
}

template<typename VertexType>
void Mesh<VertexType>::setOffset(const glm::ivec3& offset) {
	m_offset = offset;
}

template<typename VertexType>
void Mesh<VertexType>::addTriangle(IndexType index0, IndexType index1, IndexType index2) {
	//Make sure the specified indices correspond to valid vertices.
	core_assert_msg(index0 < m_vecVertices.size(), "Index points at an invalid vertex.");
	core_assert_msg(index1 < m_vecVertices.size(), "Index points at an invalid vertex.");
	core_assert_msg(index2 < m_vecVertices.size(), "Index points at an invalid vertex.");

	m_vecIndices.push_back(index0);
	m_vecIndices.push_back(index1);
	m_vecIndices.push_back(index2);
}

template<typename VertexType>
IndexType Mesh<VertexType>::addVertex(const VertexType& vertex) {
	// We should not add more vertices than our chosen index type will let us index.
	core_assert_msg(m_vecVertices.size() < std::numeric_limits<IndexType>::max(), "Mesh has more vertices that the chosen index type allows.");

	m_vecVertices.push_back(vertex);
	return m_vecVertices.size() - 1;
}

template<typename VertexType>
void Mesh<VertexType>::clear() {
	m_vecVertices.clear();
	m_vecIndices.clear();
}

template<typename VertexType>
bool Mesh<VertexType>::isEmpty() const {
	return getNoOfVertices() == 0 || getNoOfIndices() == 0;
}

template<typename VertexType>
void Mesh<VertexType>::removeUnusedVertices() {
	std::vector<bool> isVertexUsed(m_vecVertices.size());
	std::fill(isVertexUsed.begin(), isVertexUsed.end(), false);

	for (uint32_t triCt = 0; triCt < m_vecIndices.size(); triCt++) {
		int v = m_vecIndices[triCt];
		isVertexUsed[v] = true;
	}

	int noOfUsedVertices = 0;
	std::vector<uint32_t> newPos(m_vecVertices.size());
	for (size_t vertCt = 0; vertCt < m_vecVertices.size(); vertCt++) {
		if (isVertexUsed[vertCt]) {
			m_vecVertices[noOfUsedVertices] = m_vecVertices[vertCt];
			newPos[vertCt] = noOfUsedVertices;
			noOfUsedVertices++;
		}
	}

	m_vecVertices.resize(noOfUsedVertices);

	for (size_t triCt = 0; triCt < m_vecIndices.size(); triCt++) {
		m_vecIndices[triCt] = newPos[m_vecIndices[triCt]];
	}
}

}
