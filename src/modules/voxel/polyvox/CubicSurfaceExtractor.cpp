/**
 * @file
 */

#include "CubicSurfaceExtractor.h"
#include <SDL.h>

namespace voxel {

SDL_FORCE_INLINE bool isSameVertex(const VoxelVertex& v1, const VoxelVertex& v2) {
	return v1.colorIndex == v2.colorIndex && v1.ambientOcclusion == v2.ambientOcclusion;
}

static bool mergeQuads(Quad& q1, Quad& q2, Mesh* meshCurrent) {
	const VoxelVertex& v11 = meshCurrent->getVertex(q1.vertices[0]);
	const VoxelVertex& v21 = meshCurrent->getVertex(q2.vertices[0]);
	if (!isSameVertex(v11, v21)) {
		return false;
	}
	const VoxelVertex& v12 = meshCurrent->getVertex(q1.vertices[1]);
	const VoxelVertex& v22 = meshCurrent->getVertex(q2.vertices[1]);
	if (!isSameVertex(v12, v22)) {
		return false;
	}
	const VoxelVertex& v13 = meshCurrent->getVertex(q1.vertices[2]);
	const VoxelVertex& v23 = meshCurrent->getVertex(q2.vertices[2]);
	if (!isSameVertex(v13, v23)) {
		return false;
	}
	const VoxelVertex& v14 = meshCurrent->getVertex(q1.vertices[3]);
	const VoxelVertex& v24 = meshCurrent->getVertex(q2.vertices[3]);
	if (!isSameVertex(v14, v24)) {
		return false;
	}
	//Now check whether quad 2 is adjacent to quad one by comparing vertices.
	//Adjacent quads must share two vertices, and the second quad could be to the
	//top, bottom, left, of right of the first one. This gives four combinations to test.
	if (q1.vertices[0] == q2.vertices[1] && q1.vertices[3] == q2.vertices[2]) {
		q1.vertices[0] = q2.vertices[0];
		q1.vertices[3] = q2.vertices[3];
		return true;
	} else if (q1.vertices[3] == q2.vertices[0] && q1.vertices[2] == q2.vertices[1]) {
		q1.vertices[3] = q2.vertices[3];
		q1.vertices[2] = q2.vertices[2];
		return true;
	} else if (q1.vertices[1] == q2.vertices[0] && q1.vertices[2] == q2.vertices[3]) {
		q1.vertices[1] = q2.vertices[1];
		q1.vertices[2] = q2.vertices[2];
		return true;
	} else if (q1.vertices[0] == q2.vertices[3] && q1.vertices[1] == q2.vertices[2]) {
		q1.vertices[0] = q2.vertices[0];
		q1.vertices[1] = q2.vertices[1];
		return true;
	}

	// Quads cannot be merged.
	return false;
}

bool performQuadMerging(QuadList& quads, Mesh* meshCurrent) {
	bool didMerge = false;
	for (QuadList::iterator outerIter = quads.begin(); outerIter != quads.end(); ++outerIter) {
		QuadList::iterator innerIter = outerIter;
		++innerIter;
		while (innerIter != quads.end()) {
			Quad& q1 = *outerIter;
			Quad& q2 = *innerIter;

			const bool result = mergeQuads(q1, q2, meshCurrent);

			if (result) {
				didMerge = true;
				innerIter = quads.erase(innerIter);
			} else {
				++innerIter;
			}
		}
	}

	return didMerge;
}

SDL_FORCE_INLINE uint8_t vertexAmbientOcclusion(bool side1, bool side2, bool corner) {
	if (side1 && side2) {
		return 0;
	}
	return 3 - (side1 + side2 + corner);
}

IndexType addVertex(bool reuseVertices, uint32_t uX, uint32_t uY, uint32_t uZ, const Voxel& materialIn, Array& existingVertices,
		Mesh* meshCurrent, const VoxelType face1, const VoxelType face2, const VoxelType corner, const glm::ivec3& offset) {
	const uint8_t ambientOcclusion = vertexAmbientOcclusion(
		!isAir(face1) && !isWater(face1),
		!isAir(face2) && !isWater(face2),
		!isAir(corner) && !isWater(corner));

	for (uint32_t ct = 0; ct < MaxVerticesPerPosition; ++ct) {
		VertexData& entry = existingVertices(uX, uY, ct);

		if (entry.index == -1) {
			// No vertices matched and we've now hit an empty space. Fill it by creating a vertex.
			// The 0.5f offset is because vertices set between voxels in order to build cubes around them.
			// see raycastWithEndpoints for this offset, too
			VoxelVertex vertex;
			vertex.position = glm::ivec3(uX, uY, uZ) + offset;
			vertex.colorIndex = materialIn.getColor();
			vertex.material = materialIn.getMaterial();
			vertex.ambientOcclusion = ambientOcclusion;

			entry.index = meshCurrent->addVertex(vertex);
			entry.voxel = materialIn;
			entry.ambientOcclusion = vertex.ambientOcclusion;

			return entry.index;
		}

		// If we have an existing vertex and the material matches then we can return it.
		if (reuseVertices && entry.voxel.isSame(materialIn) && entry.ambientOcclusion == ambientOcclusion) {
			return entry.index;
		}
	}

	// If we exit the loop here then apparently all the slots were full but none of them matched.
	// This shouldn't ever happen, so if it does it is probably a bug in PolyVox. Please report it to us!
	core_assert_msg(false, "All slots full but no matches during cubic surface extraction. This is probably a bug in PolyVox");
	return -1; //Should never happen.
}

}
