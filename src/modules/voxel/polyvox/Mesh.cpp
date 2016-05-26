/**
 * @file
 */

#include "Mesh.h"
#include "CubicSurfaceExtractor.h"
#include "core/Common.h"
#include "core/Trace.h"

namespace voxel {

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
}

}
