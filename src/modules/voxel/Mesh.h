/**
 * @file
 */

#pragma once

#include "VoxelVertex.h"
#include "core/collection/Buffer.h"
#include <glm/vec2.hpp>

namespace voxel {

using VertexArray = core::Buffer<voxel::VoxelVertex, 1024>;
using IndexArray = core::Buffer<voxel::IndexType, 1024>;
using NormalArray = core::Buffer<glm::vec3, 1024>;
using UVArray = core::Buffer<glm::vec2, 1024>;

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
	const NormalArray& getNormalVector() const;
	const UVArray& getUVVector() const;
	IndexArray& getIndexVector();
	VertexArray& getVertexVector();
	NormalArray& getNormalVector();
	UVArray& getUVVector();

	// e.g. for transparency
	// returns true if sorting was needed
	bool sort(const glm::vec3 &cameraPos);

	const glm::ivec3& getOffset() const;
	void setOffset(const glm::ivec3& offset);

	IndexType addVertex(const VoxelVertex& vertex);
	IndexType addVertex(const VoxelVertex& vertex, const glm::vec2 &uv);
	void addTriangle(IndexType index0, IndexType index1, IndexType index2);
	void setNormal(IndexType index, const glm::vec3 &normal);

	void optimize();

	void clear();
	bool isEmpty() const;
	void removeUnusedVertices();
	void compressIndices();
	void calculateBounds();
	void calculateNormals();
	glm::vec3 mins() const { return _mins; }
	glm::vec3 maxs() const { return _maxs; }

	const uint8_t* compressedIndices() const;
	size_t compressedIndexSize() const;

	bool operator<(const Mesh& rhs) const;
private:
	alignas(16) IndexArray _vecIndices;
	alignas(16) VertexArray _vecVertices;
	alignas(16) NormalArray _normals; // marching cubes only
	alignas(16) UVArray _uvs;
	glm::highp_vec3 _mins {0};
	glm::highp_vec3 _maxs {0};
	uint8_t *_compressedIndices = nullptr;
	size_t _compressedIndexSize = 0u;
	glm::ivec3 _offset{0};
	glm::vec3 _lastCameraPos{0.0f};
	bool _mayGetResized; // just for development purposes
	int _initialVertices;
	int _initialIndices;
};

inline const uint8_t* Mesh::compressedIndices() const {
	return _compressedIndices;
}

inline size_t Mesh::compressedIndexSize() const {
	return _compressedIndexSize;
}

}
