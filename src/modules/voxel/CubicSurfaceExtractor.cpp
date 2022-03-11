/**
 * @file
 */

#include "CubicSurfaceExtractor.h"
#include "core/Common.h"

namespace voxel {

static bool isSameVertex(const VoxelVertex& v1, const VoxelVertex& v2) {
	return v1.colorIndex == v2.colorIndex && v1.info == v2.info;
}

static bool isSameColor(const VoxelVertex& v1, const VoxelVertex& v2) {
	return v1.colorIndex == v2.colorIndex;
}

template<class FUNC>
static bool mergeQuads(Quad& q1, Quad& q2, Mesh* meshCurrent, FUNC&& equal) {
	core_trace_scoped(MergeQuads);
	const VertexArray& vv = meshCurrent->getVertexVector();
	const VoxelVertex& v11 = vv[q1.vertices[0]];
	const VoxelVertex& v21 = vv[q2.vertices[0]];
	if (!equal(v11, v21)) {
		return false;
	}
	const VoxelVertex& v12 = vv[q1.vertices[1]];
	const VoxelVertex& v22 = vv[q2.vertices[1]];
	if (!equal(v12, v22)) {
		return false;
	}
	const VoxelVertex& v13 = vv[q1.vertices[2]];
	const VoxelVertex& v23 = vv[q2.vertices[2]];
	if (!equal(v13, v23)) {
		return false;
	}
	const VoxelVertex& v14 = vv[q1.vertices[3]];
	const VoxelVertex& v24 = vv[q2.vertices[3]];
	if (!equal(v14, v24)) {
		return false;
	}
	//Now check whether quad 2 is adjacent to quad one by comparing vertices.
	//Adjacent quads must share two vertices, and the second quad could be to the
	//top, bottom, left, of right of the first one. This gives four combinations to test.
	if (q1.vertices[0] == q2.vertices[1] && q1.vertices[3] == q2.vertices[2]) {
		q1.vertices[0] = q2.vertices[0];
		q1.vertices[3] = q2.vertices[3];
		return true;
	}
	if (q1.vertices[3] == q2.vertices[0] && q1.vertices[2] == q2.vertices[1]) {
		q1.vertices[3] = q2.vertices[3];
		q1.vertices[2] = q2.vertices[2];
		return true;
	}
	if (q1.vertices[1] == q2.vertices[0] && q1.vertices[2] == q2.vertices[3]) {
		q1.vertices[1] = q2.vertices[1];
		q1.vertices[2] = q2.vertices[2];
		return true;
	}
	if (q1.vertices[0] == q2.vertices[3] && q1.vertices[1] == q2.vertices[2]) {
		q1.vertices[0] = q2.vertices[0];
		q1.vertices[1] = q2.vertices[1];
		return true;
	}

	// Quads cannot be merged.
	return false;
}

static bool performQuadMerging(QuadList& quads, Mesh* meshCurrent, bool ambientOcclusion) {
	core_trace_scoped(PerformQuadMerging);
	bool didMerge = false;

	auto* equal = isSameVertex;
	if (!ambientOcclusion) {
		equal = isSameColor;
	}

	for (QuadList::iterator outerIter = quads.begin(); outerIter != quads.end(); ++outerIter) {
		QuadList::iterator innerIter = outerIter;
		++innerIter;
		while (innerIter != quads.end()) {
			Quad& q1 = *outerIter;
			Quad& q2 = *innerIter;

			const bool result = mergeQuads(q1, q2, meshCurrent, equal);

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

/**
 * @brief We are checking the voxels above us. There are four possible ambient occlusion values
 * for a vertex.
 */
SDL_FORCE_INLINE uint8_t vertexAmbientOcclusion(bool side1, bool side2, bool corner) {
	if (side1 && side2) {
		return 0;
	}
	return 3 - (side1 + side2 + corner);
}

/**
 * @note Notice that the ambient occlusion is different for the vertices on the side than it is for the
 * vertices on the top and bottom. To fix this, we just need to pick a consistent orientation for
 * the quads. This can be done by comparing the ambient occlusion values for each quad and selecting
 * an appropriate orientation. Quad vertices must be sorted in clockwise order.
 */
SDL_FORCE_INLINE bool isQuadFlipped(const VoxelVertex& v00, const VoxelVertex& v01, const VoxelVertex& v10, const VoxelVertex& v11) {
	return v00.ambientOcclusion + v11.ambientOcclusion > v01.ambientOcclusion + v10.ambientOcclusion;
}

void meshify(Mesh* result, bool mergeQuads, bool ambientOcclusion, QuadListVector& vecListQuads) {
	core_trace_scoped(GenerateMeshify);
	for (QuadList& listQuads : vecListQuads) {
		if (mergeQuads) {
			core_trace_scoped(MergeQuads);
			// Repeatedly call this function until it returns
			// false to indicate nothing more can be done.
			while (performQuadMerging(listQuads, result, ambientOcclusion)) {
			}
		}

		for (const Quad& quad : listQuads) {
			const IndexType i0 = quad.vertices[0];
			const IndexType i1 = quad.vertices[1];
			const IndexType i2 = quad.vertices[2];
			const IndexType i3 = quad.vertices[3];
			const VoxelVertex& v00 = result->getVertex(i3);
			const VoxelVertex& v01 = result->getVertex(i0);
			const VoxelVertex& v10 = result->getVertex(i2);
			const VoxelVertex& v11 = result->getVertex(i1);

			if (isQuadFlipped(v00, v01, v10, v11)) {
				result->addTriangle(i1, i2, i3);
				result->addTriangle(i1, i3, i0);
			} else {
				result->addTriangle(i0, i1, i2);
				result->addTriangle(i0, i2, i3);
			}
		}
	}
}

IndexType addVertex(bool reuseVertices, uint32_t x, uint32_t y, uint32_t z, const Voxel& materialIn, Array& existingVertices,
		Mesh* meshCurrent, const VoxelType face1, const VoxelType face2, const VoxelType corner, const glm::ivec3& offset) {
	core_trace_scoped(AddVertex);
	const uint8_t ambientOcclusion = vertexAmbientOcclusion(
		!isAir(face1) && !isTransparent(face1),
		!isAir(face2) && !isTransparent(face2),
		!isAir(corner) && !isTransparent(corner));

	for (uint32_t ct = 0; ct < MaxVerticesPerPosition; ++ct) {
		VertexData& entry = existingVertices(x, y, ct);

		if (entry.index == 0) {
			// No vertices matched and we've now hit an empty space. Fill it by creating a vertex.
			VoxelVertex vertex;
			vertex.position = glm::ivec3(x, y, z) + offset;
			vertex.colorIndex = materialIn.getColor();
			vertex.ambientOcclusion = ambientOcclusion;
			vertex.flags = materialIn.getFlags();
			vertex.padding = 0u;

			entry.index = (int32_t)meshCurrent->addVertex(vertex) + 1;
			entry.voxel = materialIn;
			entry.ambientOcclusion = vertex.ambientOcclusion;

			return entry.index - 1;
		}

		// If we have an existing vertex and the material matches then we can return it.
		if (reuseVertices && entry.ambientOcclusion == ambientOcclusion && entry.voxel.getFlags() == materialIn.getFlags() && entry.voxel.isSame(materialIn)) {
			return entry.index - 1;
		}
	}

	// If we exit the loop here then apparently all the slots were full but none of them matched.
	// This shouldn't ever happen, so if it does it is probably a bug in PolyVox. Please report it to us!
	core_assert_msg(false, "All slots full but no matches during cubic surface extraction. This is probably a bug in PolyVox");
	return 0; //Should never happen.
}

}
