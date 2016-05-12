/**
 * @file
 */

#include "CubicSurfaceExtractor.h"

namespace voxel {

bool mergeQuads(Quad& q1, Quad& q2, Mesh<Vertex>* m_meshCurrent) {
	//All four vertices of a given quad have the same data,
	//so just check that the first pair of vertices match.
	if (m_meshCurrent->getVertex(q1.vertices[0]).data == m_meshCurrent->getVertex(q2.vertices[0]).data) {
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
	}

	//Quads cannot be merged.
	return false;
}

bool performQuadMerging(std::list<Quad>& quads, Mesh<Vertex>* m_meshCurrent) {
	bool bDidMerge = false;
	for (typename std::list<Quad>::iterator outerIter = quads.begin(); outerIter != quads.end(); outerIter++) {
		typename std::list<Quad>::iterator innerIter = outerIter;
		innerIter++;
		while (innerIter != quads.end()) {
			Quad& q1 = *outerIter;
			Quad& q2 = *innerIter;

			bool result = mergeQuads(q1, q2, m_meshCurrent);

			if (result) {
				bDidMerge = true;
				innerIter = quads.erase(innerIter);
			} else {
				innerIter++;
			}
		}
	}

	return bDidMerge;
}

int32_t addVertex(uint32_t uX, uint32_t uY, uint32_t uZ, const Voxel& uMaterialIn, Array<3, IndexAndMaterial>& existingVertices,
		Mesh<Vertex>* m_meshCurrent, const Voxel& face1, const Voxel& face2, const Voxel& corner) {
	for (uint32_t ct = 0; ct < MaxVerticesPerPosition; ct++) {
		IndexAndMaterial& rEntry = existingVertices(uX, uY, ct);

		if (rEntry.iIndex == -1 || true) {
			// No vertices matched and we've now hit an empty space. Fill it by creating a vertex.
			// The 0.5f offset is because vertices set between voxels in order to build cubes around them.
			Vertex Vertex;
			Vertex.position = { uX, uY, uZ };
			Vertex.data = uMaterialIn;
			Vertex.ambientOcclusion = vertexAmbientOcclusion(
				face1.getMaterial() != voxel::Air,
				face2.getMaterial() != voxel::Air,
				corner.getMaterial() != voxel::Air);
			rEntry.iIndex = m_meshCurrent->addVertex(Vertex);
			rEntry.uMaterial = uMaterialIn;

			return rEntry.iIndex;
		}

		// If we have an existing vertex and the material matches then we can return it.
		if (rEntry.uMaterial == uMaterialIn) {
			return rEntry.iIndex;
		}
	}

	// If we exit the loop here then apparently all the slots were full but none of them matched.
	// This shouldn't ever happen, so if it does it is probably a bug in PolyVox. Please report it to us!
	core_assert_msg(false, "All slots full but no matches during cubic surface extraction. This is probably a bug in PolyVox");
	return -1; //Should never happen.
}

}
