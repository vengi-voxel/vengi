/**
 * @file
 */

#include "CubicSurfaceExtractor.h"

namespace voxel {

inline bool isSameVertex(const Vertex& v1, const Vertex& v2) {
	return v1.data == v2.data && v1.ambientOcclusion == v2.ambientOcclusion;
}

bool mergeQuads(Quad& q1, Quad& q2, Mesh<Vertex>* m_meshCurrent) {
	const Vertex& v11 = m_meshCurrent->getVertex(q1.vertices[0]);
	const Vertex& v21 = m_meshCurrent->getVertex(q2.vertices[0]);
	const Vertex& v12 = m_meshCurrent->getVertex(q1.vertices[1]);
	const Vertex& v22 = m_meshCurrent->getVertex(q2.vertices[1]);
	const Vertex& v13 = m_meshCurrent->getVertex(q1.vertices[2]);
	const Vertex& v23 = m_meshCurrent->getVertex(q2.vertices[2]);
	const Vertex& v14 = m_meshCurrent->getVertex(q1.vertices[3]);
	const Vertex& v24 = m_meshCurrent->getVertex(q2.vertices[3]);
	if (isSameVertex(v11, v21) && isSameVertex(v12, v22) && isSameVertex(v13, v23) && isSameVertex(v14, v24)) {
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
	for (typename std::list<Quad>::iterator outerIter = quads.begin(); outerIter != quads.end(); ++outerIter) {
		typename std::list<Quad>::iterator innerIter = outerIter;
		++innerIter;
		while (innerIter != quads.end()) {
			Quad& q1 = *outerIter;
			Quad& q2 = *innerIter;

			const bool result = mergeQuads(q1, q2, m_meshCurrent);

			if (result) {
				bDidMerge = true;
				innerIter = quads.erase(innerIter);
			} else {
				++innerIter;
			}
		}
	}

	return bDidMerge;
}

int32_t addVertex(uint32_t uX, uint32_t uY, uint32_t uZ, const Voxel& uMaterialIn, Array<3, VertexData>& existingVertices,
		Mesh<Vertex>* m_meshCurrent, const Voxel& face1, const Voxel& face2, const Voxel& corner) {
	for (uint32_t ct = 0; ct < MaxVerticesPerPosition; ct++) {
		VertexData& rEntry = existingVertices(uX, uY, ct);

		const uint8_t ambientOcclusion = vertexAmbientOcclusion(
			face1.getMaterial() != voxel::Air && face1.getMaterial() != voxel::Water,
			face2.getMaterial() != voxel::Air && face2.getMaterial() != voxel::Water,
			corner.getMaterial() != voxel::Air && corner.getMaterial() != voxel::Water);

		if (rEntry.iIndex == -1) {
			// No vertices matched and we've now hit an empty space. Fill it by creating a vertex.
			// The 0.5f offset is because vertices set between voxels in order to build cubes around them.
			// see raycastWithEndpoints for this offset, too
			Vertex vertex;
			vertex.position = { uX, uY, uZ };
			vertex.data = uMaterialIn;
			vertex.ambientOcclusion = ambientOcclusion;

			rEntry.iIndex = m_meshCurrent->addVertex(vertex);
			rEntry.uMaterial = uMaterialIn;
			rEntry.ambientOcclusion = vertex.ambientOcclusion;

			return rEntry.iIndex;
		}

		// If we have an existing vertex and the material matches then we can return it.
		if (rEntry.uMaterial == uMaterialIn && rEntry.ambientOcclusion == ambientOcclusion) {
			return rEntry.iIndex;
		}
	}

	// If we exit the loop here then apparently all the slots were full but none of them matched.
	// This shouldn't ever happen, so if it does it is probably a bug in PolyVox. Please report it to us!
	core_assert_msg(false, "All slots full but no matches during cubic surface extraction. This is probably a bug in PolyVox");
	return -1; //Should never happen.
}

}
