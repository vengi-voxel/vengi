/**
 * @file
 */

#include "Mesh.h"
#include "CubicSurfaceExtractor.h"
#include "core/Common.h"
#include "core/Trace.h"

namespace voxel {

size_t Mesh::size() {
	constexpr size_t classSize = sizeof(*this);
	const size_t indicesSize = _vecIndices.size() * sizeof(IndexType);
	const size_t verticesSize = _vecVertices.size() * sizeof(VoxelVertex);
	const size_t contentSize = indicesSize + verticesSize;
	return classSize + contentSize;
}

bool Mesh::addMesh(const Mesh& mesh) {
	if (mesh.getOffset() != getOffset()) {
		return false;
	}
	const IndexType* indices = mesh.getRawIndexData();
	const VoxelVertex* vertices = mesh.getRawVertexData();
	const size_t nIndices = mesh.getNoOfIndices();
	const size_t nVertices = mesh.getNoOfVertices();

	const size_t vSize = _vecVertices.size();
	const size_t iSize = _vecIndices.size();

	_vecVertices.reserve(vSize + nVertices);
	_vecIndices.reserve(iSize + nIndices);

	for (size_t i = 0; i < nVertices; ++i) {
		_vecVertices.push_back(vertices[i]);
	}
	for (size_t i = 0; i < nIndices; ++i) {
		// offset by the already added vertices
		_vecIndices.push_back(indices[i] + vSize);
	}

	return true;
}

void Mesh::removeUnusedVertices() {
	std::vector<bool> isVertexUsed(_vecVertices.size());
	std::fill(isVertexUsed.begin(), isVertexUsed.end(), false);

	for (uint32_t triCt = 0; triCt < _vecIndices.size(); triCt++) {
		int v = _vecIndices[triCt];
		isVertexUsed[v] = true;
	}

	int noOfUsedVertices = 0;
	std::vector<uint32_t> newPos(_vecVertices.size());
	for (size_t vertCt = 0; vertCt < _vecVertices.size(); vertCt++) {
		if (isVertexUsed[vertCt]) {
			_vecVertices[noOfUsedVertices] = _vecVertices[vertCt];
			newPos[vertCt] = noOfUsedVertices;
			noOfUsedVertices++;
		}
	}

	_vecVertices.resize(noOfUsedVertices);

	for (size_t triCt = 0; triCt < _vecIndices.size(); triCt++) {
		_vecIndices[triCt] = newPos[_vecIndices[triCt]];
	}
	_vecIndices.resize(_vecIndices.size());
}

}
