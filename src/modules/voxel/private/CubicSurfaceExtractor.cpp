/**
 * @file
 */

#include "CubicSurfaceExtractor.h"
#include "app/Async.h"
#include "core/Common.h"
#include "core/concurrent/Atomic.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelVertex.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/Enum.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/NonCopyable.h"
#include "voxel/Region.h"
#include "core/Trace.h"
#include "voxel/Face.h"
#include <glm/vec3.hpp>
#include <list>
#include <vector>

namespace voxel {

/**
 * This constant defines the maximum number of quads which can share a vertex in a cubic style mesh.
 *
 * We try to avoid duplicate vertices by checking whether a vertex has already been added at a given position.
 * However, it is possible that vertices have the same position but different materials. In this case, the
 * vertices are not true duplicates and both must be added to the mesh. As far as I can tell, it is possible to have
 * at most eight vertices with the same position but different materials. For example, this worst-case scenario
 * happens when we have a 2x2x2 group of voxels, all with different materials and some/all partially transparent.
 * The vertex position at the center of this group is then going to be used by all eight voxels all with different
 * materials.
 */
const uint32_t MaxVerticesPerPosition = 8;

/**
 * @section Data structures
 */

struct Quad {
	inline Quad(IndexType v0, IndexType v1, IndexType v2, IndexType v3) : vertices{v0, v1, v2, v3} {
	}

	IndexType vertices[4];
};

struct VertexData {
	int32_t index;
	Voxel voxel;
	uint8_t ambientOcclusion;
};
static_assert(sizeof(VertexData) == 12, "Unexpected size of VertexData");

class Array : public core::NonCopyable {
private:
	uint32_t _width;
	uint32_t _height;
	uint32_t _depth;
	VertexData* _elements;
public:
	Array(uint32_t width, uint32_t height, uint32_t depth) :
			_width(width), _height(height), _depth(depth) {
		_elements = (VertexData*)core_malloc(width * height * depth * sizeof(VertexData));
		clear();
	}

	~Array() {
		core_free(_elements);
	}

	void clear() {
		core_memset((void*)_elements, 0x0, _width * _height * _depth * sizeof(VertexData));
	}

	inline VertexData& operator()(uint32_t x, uint32_t y, uint32_t z) {
		core_assert_msg(x < _width && y < _height && z < _depth, "Array access is out-of-range.");
		return _elements[z * _width * _height + y * _width + x];
	}

	void swap(Array& other) {
		core::exchange(_elements, other._elements);
	}
};

/**
 * @brief Should be a list because random inserts which we need in @c performQuadMerging are O(1)
 */
typedef std::list<Quad> QuadList;
typedef std::vector<QuadList> QuadListVector;

/**
 * @section Surface extraction
 */

/**
 * @brief implementation of a function object for deciding when
 * the cubic surface extractor should insert a face between two voxels.
 *
 * The criteria used here are that the voxel in front of the potential
 * quad should have a value of zero (which would typically indicate empty
 * space) while the voxel behind the potential quad would have a value
 * greater than zero (typically indicating it is solid).
 */
static CORE_FORCE_INLINE bool isQuadNeeded(VoxelType back, VoxelType front, FaceNames face) {
	return back == VoxelType::Generic && front <= VoxelType::Transparent;
}

static CORE_FORCE_INLINE bool isTransparentQuadNeeded(VoxelType back, VoxelType front, FaceNames face) {
	if (!isTransparent(back)) {
		return false;
	}
	return !isTransparent(front);
}

static CORE_FORCE_INLINE bool isSameVertex(const VoxelVertex& v1, const VoxelVertex& v2) {
	return v1.colorIndex == v2.colorIndex && v1.info == v2.info && v1.normalIndex == v2.normalIndex;
}

static CORE_FORCE_INLINE bool isSameColor(const VoxelVertex& v1, const VoxelVertex& v2) {
	return v1.colorIndex == v2.colorIndex;
}

static bool mergeQuads(Quad& q1, Quad& q2, Mesh* meshCurrent) {
	core_trace_scoped(MergeQuads);
	const VertexArray& vv = meshCurrent->getVertexVector();
	const VoxelVertex& v11 = vv[q1.vertices[0]];
	const VoxelVertex& v21 = vv[q2.vertices[0]];
	if (!isSameColor(v11, v21)) {
		return false;
	}
	const VoxelVertex& v12 = vv[q1.vertices[1]];
	const VoxelVertex& v22 = vv[q2.vertices[1]];
	if (!isSameColor(v12, v22)) {
		return false;
	}
	const VoxelVertex& v13 = vv[q1.vertices[2]];
	const VoxelVertex& v23 = vv[q2.vertices[2]];
	if (!isSameColor(v13, v23)) {
		return false;
	}
	const VoxelVertex& v14 = vv[q1.vertices[3]];
	const VoxelVertex& v24 = vv[q2.vertices[3]];
	if (!isSameColor(v14, v24)) {
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

static bool mergeQuadsAO(Quad& q1, Quad& q2, Mesh* meshCurrent) {
	core_trace_scoped(MergeQuads);
	const VertexArray& vv = meshCurrent->getVertexVector();
	const VoxelVertex& v11 = vv[q1.vertices[0]];
	const VoxelVertex& v21 = vv[q2.vertices[0]];
	if (!isSameVertex(v11, v21)) {
		return false;
	}
	const VoxelVertex& v12 = vv[q1.vertices[1]];
	const VoxelVertex& v22 = vv[q2.vertices[1]];
	if (!isSameVertex(v12, v22)) {
		return false;
	}
	const VoxelVertex& v13 = vv[q1.vertices[2]];
	const VoxelVertex& v23 = vv[q2.vertices[2]];
	if (!isSameVertex(v13, v23)) {
		return false;
	}
	const VoxelVertex& v14 = vv[q1.vertices[3]];
	const VoxelVertex& v24 = vv[q2.vertices[3]];
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

template<bool AmbientOcclusion>
static bool performQuadMerging(QuadList &quads, Mesh *meshCurrent) {
	if (quads.empty()) {
		return false;
	}
	core_trace_scoped(PerformQuadMerging);

	if (quads.size() > 1000) {
		const size_t nBuckets = (quads.size() + 1000 - 1) / 1000;
		std::vector<QuadList> buckets(nBuckets);
		auto it = quads.begin();
		const size_t bucketSize = quads.size() / nBuckets;
		for (size_t i = 0; i < nBuckets; ++i) {
			auto end = it;
			if (i == nBuckets - 1) {
				end = quads.end();
			} else {
				std::advance(end, bucketSize);
			}
			buckets[i].splice(buckets[i].begin(), quads, it, end);
			it = end;
		}

		core::AtomicBool didMergeAny{false};
		app::for_parallel(0, (int)nBuckets, [&](int start, int end) {
			for (int i = start; i < end; ++i) {
				if (performQuadMerging<AmbientOcclusion>(buckets[i], meshCurrent)) {
					didMergeAny = true;
				}
			}
		});

		for (auto &bucket : buckets) {
			quads.splice(quads.end(), bucket);
		}
		return didMergeAny;
	}
	bool didMerge = false;

	// TODO: PERF: this loop is eliminating duplicates in O(n^2)
	// we could do better with building an edge map or similar structure
	// and only merge quads that share an edge
	Log::trace("Merge quads: starting with %i quads", (int)quads.size());
	for (QuadList::iterator outerIter = quads.begin(); outerIter != quads.end(); ++outerIter) {
		QuadList::iterator innerIter = outerIter;
		++innerIter;
		while (innerIter != quads.end()) {
			Quad& q1 = *outerIter;
			Quad& q2 = *innerIter;

			bool result;
			if constexpr (AmbientOcclusion) {
				result = mergeQuadsAO(q1, q2, meshCurrent);
			} else {
				result = mergeQuads(q1, q2, meshCurrent);
			}

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
 *
 * @li https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
 * @li https://www.reddit.com/r/gamedev/comments/1gvk18/comment/cao9n91/
 *
 * There are four states for the ambient occlusion value.
 * @li @c 0: full occluded in the corner of three voxels
 * @li @c 3: no occluded at all
 */
CORE_FORCE_INLINE uint8_t vertexAmbientOcclusion(bool side1, bool side2, bool corner) {
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
CORE_FORCE_INLINE bool isQuadFlipped(const VoxelVertex& v00, const VoxelVertex& v01, const VoxelVertex& v10, const VoxelVertex& v11) {
	return v00.ambientOcclusion + v11.ambientOcclusion > v01.ambientOcclusion + v10.ambientOcclusion;
}

static void meshify(Mesh* result, bool mergeQuads, bool ambientOcclusion, QuadListVector& vecListQuads) {
	core_trace_scoped(GenerateMeshify);
	for (QuadList& listQuads : vecListQuads) {
		if (mergeQuads) {
			core_trace_scoped(MergeQuads);
			// Repeatedly call this function until it returns
			// false to indicate nothing more can be done.
			if (ambientOcclusion) {
				while (performQuadMerging<true>(listQuads, result)) {
				}
			} else {
				while (performQuadMerging<false>(listQuads, result)) {
				}
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

static IndexType addVertex(bool reuseVertices, uint32_t x, uint32_t y, uint32_t z, const Voxel& materialIn, Array& existingVertices,
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
			vertex.normalIndex = materialIn.getNormal();
			vertex.ambientOcclusion = ambientOcclusion;
			vertex.flags = materialIn.getFlags();
			vertex.padding = 0u; // Voxel::_unused
			vertex.padding2 = 0u;

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

void extractCubicMesh(const voxel::RawVolume *volData, const Region &region, ChunkMesh *result,
					  const glm::ivec3 &translate, bool ambientOcclusion, bool mergeQuads, bool reuseVertices) {
	core_trace_scoped(ExtractCubicMesh);

	const glm::ivec3& offset = region.getLowerCorner();
	const glm::ivec3& upper = region.getUpperCorner();
	result->setOffset(offset);

	// Used to avoid creating duplicate vertices.
	const int widthInCells = upper.x - offset.x;
	const int heightInCells = upper.y - offset.y;
	Array previousSliceVertices(widthInCells + 2, heightInCells + 2, MaxVerticesPerPosition);
	Array currentSliceVertices(widthInCells + 2, heightInCells + 2, MaxVerticesPerPosition);

	Array previousSliceVerticesT(widthInCells + 2, heightInCells + 2, MaxVerticesPerPosition);
	Array currentSliceVerticesT(widthInCells + 2, heightInCells + 2, MaxVerticesPerPosition);

	// During extraction we create a number of different lists of quads. All the
	// quads in a given list are in the same plane and facing in the same direction.
	QuadListVector vecQuads[core::enumVal(FaceNames::Max)];
	QuadListVector vecQuadsT[core::enumVal(FaceNames::Max)];

	const int32_t xSize = upper.x - offset.x + 2;
	const int32_t ySize = upper.y - offset.y + 2;
	const int32_t zSize = upper.z - offset.z + 2;
	vecQuads[core::enumVal(FaceNames::NegativeX)].resize(xSize);
	vecQuads[core::enumVal(FaceNames::PositiveX)].resize(xSize);
	vecQuadsT[core::enumVal(FaceNames::NegativeX)].resize(xSize);
	vecQuadsT[core::enumVal(FaceNames::PositiveX)].resize(xSize);

	vecQuads[core::enumVal(FaceNames::NegativeY)].resize(ySize);
	vecQuads[core::enumVal(FaceNames::PositiveY)].resize(ySize);
	vecQuadsT[core::enumVal(FaceNames::NegativeY)].resize(ySize);
	vecQuadsT[core::enumVal(FaceNames::PositiveY)].resize(ySize);

	vecQuads[core::enumVal(FaceNames::NegativeZ)].resize(zSize);
	vecQuads[core::enumVal(FaceNames::PositiveZ)].resize(zSize);
	vecQuadsT[core::enumVal(FaceNames::NegativeZ)].resize(zSize);
	vecQuadsT[core::enumVal(FaceNames::PositiveZ)].resize(zSize);

	voxel::RawVolume::Sampler volumeSampler(volData);

	{
	core_trace_scoped(QuadGeneration);
	volumeSampler.setPosition(offset);

	const uint32_t w = upper.x - offset.x;
	const uint32_t h = upper.y - offset.y;
	const uint32_t d = upper.z - offset.z;

	for (uint32_t regZ = 0; regZ <= d; ++regZ) {
		voxel::RawVolume::Sampler volumeSampler2 = volumeSampler;
		for (uint32_t regY = 0; regY <= h; ++regY) {
			voxel::RawVolume::Sampler volumeSampler3 = volumeSampler2;
			for (uint32_t regX = 0; regX <= w; ++regX) {

				/**
				 *
				 *
				 *                  [D]
				 *            8 ____________ 7
				 *             /|          /|
				 *            / |         / |              ABOVE [D] |
				 *           /  |    [F] /  |              BELOW [C]
				 *        5 /___|_______/ 6 |  [B]       y           BEHIND  [F]
				 *    [A]   |   |_______|___|              |      z  BEFORE [E] /
				 *          | 4 /       |   / 3            |   /
				 *          |  / [E]    |  /               |  /   . center
				 *          | /         | /                | /
				 *          |/__________|/                 |/________   LEFT  RIGHT
				 *        1               2                          x   [A] - [B]
				 *               [C]
				 */

				const Voxel& voxelCurrent          = volumeSampler3.voxel();
				const Voxel& voxelLeft             = volumeSampler3.peekVoxel1nx0py0pz();
				const Voxel& voxelBefore           = volumeSampler3.peekVoxel0px0py1nz();
				const Voxel& voxelLeftBefore       = volumeSampler3.peekVoxel1nx0py1nz();
				const Voxel& voxelRightBefore      = volumeSampler3.peekVoxel1px0py1nz();
				const Voxel& voxelLeftBehind       = volumeSampler3.peekVoxel1nx0py1pz();

				const Voxel& voxelAboveLeft        = volumeSampler3.peekVoxel1nx1py0pz();
				const Voxel& voxelAboveBefore      = volumeSampler3.peekVoxel0px1py1nz();
				const Voxel& voxelAboveLeftBefore  = volumeSampler3.peekVoxel1nx1py1nz();
				const Voxel& voxelAboveRightBefore = volumeSampler3.peekVoxel1px1py1nz();
				const Voxel& voxelAboveLeftBehind  = volumeSampler3.peekVoxel1nx1py1pz();

				const Voxel& voxelBelow            = volumeSampler3.peekVoxel0px1ny0pz();
				const Voxel& voxelBelowLeft        = volumeSampler3.peekVoxel1nx1ny0pz();
				const Voxel& voxelBelowBefore      = volumeSampler3.peekVoxel0px1ny1nz();
				const Voxel& voxelBelowLeftBefore  = volumeSampler3.peekVoxel1nx1ny1nz();
				const Voxel& voxelBelowRightBefore = volumeSampler3.peekVoxel1px1ny1nz();
				const Voxel& voxelBelowLeftBehind  = volumeSampler3.peekVoxel1nx1ny1pz();

				const VoxelType voxelCurrentMaterial          = voxelCurrent.getMaterial();
				const VoxelType voxelLeftMaterial             = voxelLeft.getMaterial();
				const VoxelType voxelBelowMaterial            = voxelBelow.getMaterial();
				const VoxelType voxelBeforeMaterial           = voxelBefore.getMaterial();
				const VoxelType voxelLeftBeforeMaterial       = voxelLeftBefore.getMaterial();
				const VoxelType voxelBelowLeftMaterial        = voxelBelowLeft.getMaterial();
				const VoxelType voxelBelowLeftBeforeMaterial  = voxelBelowLeftBefore.getMaterial();
				const VoxelType voxelLeftBehindMaterial       = voxelLeftBehind.getMaterial();
				const VoxelType voxelBelowLeftBehindMaterial  = voxelBelowLeftBehind.getMaterial();
				const VoxelType voxelAboveLeftMaterial        = voxelAboveLeft.getMaterial();
				const VoxelType voxelAboveLeftBehindMaterial  = voxelAboveLeftBehind.getMaterial();
				const VoxelType voxelAboveLeftBeforeMaterial  = voxelAboveLeftBefore.getMaterial();

				// X [A] LEFT
				if (isQuadNeeded(voxelCurrentMaterial, voxelLeftMaterial, FaceNames::NegativeX)) {
					const IndexType v_0_1 = addVertex(reuseVertices, regX, regY,     regZ,     voxelCurrent, previousSliceVertices, &result->mesh[0],
							voxelLeftBeforeMaterial, voxelBelowLeftMaterial, voxelBelowLeftBeforeMaterial, translate);
					const IndexType v_1_4 = addVertex(reuseVertices, regX, regY,     regZ + 1, voxelCurrent, currentSliceVertices,  &result->mesh[0],
							voxelBelowLeftMaterial, voxelLeftBehindMaterial, voxelBelowLeftBehindMaterial, translate);
					const IndexType v_2_8 = addVertex(reuseVertices, regX, regY + 1, regZ + 1, voxelCurrent, currentSliceVertices,  &result->mesh[0],
							voxelLeftBehindMaterial, voxelAboveLeftMaterial, voxelAboveLeftBehindMaterial, translate);
					const IndexType v_3_5 = addVertex(reuseVertices, regX, regY + 1, regZ,     voxelCurrent, previousSliceVertices, &result->mesh[0],
							voxelAboveLeftMaterial, voxelLeftBeforeMaterial, voxelAboveLeftBeforeMaterial, translate);
					vecQuads[core::enumVal(FaceNames::NegativeX)][regX].emplace_back(v_0_1, v_1_4, v_2_8, v_3_5);
				} else if (isTransparentQuadNeeded(voxelCurrentMaterial, voxelLeftMaterial, FaceNames::NegativeX)) {
					const IndexType v_0_1 = addVertex(reuseVertices, regX, regY,     regZ,     voxelCurrent, previousSliceVerticesT, &result->mesh[1],
							voxelLeftBeforeMaterial, voxelBelowLeftMaterial, voxelBelowLeftBeforeMaterial, translate);
					const IndexType v_1_4 = addVertex(reuseVertices, regX, regY,     regZ + 1, voxelCurrent, currentSliceVerticesT,  &result->mesh[1],
							voxelBelowLeftMaterial, voxelLeftBehindMaterial, voxelBelowLeftBehindMaterial, translate);
					const IndexType v_2_8 = addVertex(reuseVertices, regX, regY + 1, regZ + 1, voxelCurrent, currentSliceVerticesT,  &result->mesh[1],
							voxelLeftBehindMaterial, voxelAboveLeftMaterial, voxelAboveLeftBehindMaterial, translate);
					const IndexType v_3_5 = addVertex(reuseVertices, regX, regY + 1, regZ,     voxelCurrent, previousSliceVerticesT, &result->mesh[1],
							voxelAboveLeftMaterial, voxelLeftBeforeMaterial, voxelAboveLeftBeforeMaterial, translate);
					vecQuadsT[core::enumVal(FaceNames::NegativeX)][regX].emplace_back(v_0_1, v_1_4, v_2_8, v_3_5);
				}

				// X [B] RIGHT
				if (isQuadNeeded(voxelLeftMaterial, voxelCurrentMaterial, FaceNames::PositiveX)) {
					const VoxelType _voxelRightBehind      = volumeSampler3.peekVoxel0px0py1pz().getMaterial();
					const VoxelType _voxelAboveRight       = volumeSampler3.peekVoxel0px1py0pz().getMaterial();
					const VoxelType _voxelAboveRightBehind = volumeSampler3.peekVoxel0px1py1pz().getMaterial();
					const VoxelType _voxelBelowRightBehind = volumeSampler3.peekVoxel0px1ny1pz().getMaterial();

					const VoxelType _voxelAboveRightBefore = voxelAboveBefore.getMaterial();
					const VoxelType _voxelBelowRightBefore = voxelBelowBefore.getMaterial();

					const IndexType v_0_2 = addVertex(reuseVertices, regX, regY,     regZ,     voxelLeft, previousSliceVertices, &result->mesh[0],
							voxelBelowMaterial, voxelBeforeMaterial, _voxelBelowRightBefore, translate);
					const IndexType v_1_3 = addVertex(reuseVertices, regX, regY,     regZ + 1, voxelLeft, currentSliceVertices,  &result->mesh[0],
							voxelBelowMaterial, _voxelRightBehind, _voxelBelowRightBehind, translate);
					const IndexType v_2_7 = addVertex(reuseVertices, regX, regY + 1, regZ + 1, voxelLeft, currentSliceVertices,  &result->mesh[0],
							_voxelAboveRight, _voxelRightBehind, _voxelAboveRightBehind, translate);
					const IndexType v_3_6 = addVertex(reuseVertices, regX, regY + 1, regZ,     voxelLeft, previousSliceVertices, &result->mesh[0],
							_voxelAboveRight, voxelBeforeMaterial, _voxelAboveRightBefore, translate);
					vecQuads[core::enumVal(FaceNames::PositiveX)][regX].emplace_back(v_0_2, v_3_6, v_2_7, v_1_3);
				} else if (isTransparentQuadNeeded(voxelLeftMaterial, voxelCurrentMaterial, FaceNames::PositiveX)) {
					const VoxelType _voxelRightBehind      = volumeSampler3.peekVoxel0px0py1pz().getMaterial();
					const VoxelType _voxelAboveRight       = volumeSampler3.peekVoxel0px1py0pz().getMaterial();
					const VoxelType _voxelAboveRightBehind = volumeSampler3.peekVoxel0px1py1pz().getMaterial();
					const VoxelType _voxelBelowRightBehind = volumeSampler3.peekVoxel0px1ny1pz().getMaterial();

					const VoxelType _voxelAboveRightBefore = voxelAboveBefore.getMaterial();
					const VoxelType _voxelBelowRightBefore = voxelBelowBefore.getMaterial();

					const IndexType v_0_2 = addVertex(reuseVertices, regX, regY,     regZ,     voxelLeft, previousSliceVerticesT, &result->mesh[1],
							voxelBelowMaterial, voxelBeforeMaterial, _voxelBelowRightBefore, translate);
					const IndexType v_1_3 = addVertex(reuseVertices, regX, regY,     regZ + 1, voxelLeft, currentSliceVerticesT,  &result->mesh[1],
							voxelBelowMaterial, _voxelRightBehind, _voxelBelowRightBehind, translate);
					const IndexType v_2_7 = addVertex(reuseVertices, regX, regY + 1, regZ + 1, voxelLeft, currentSliceVerticesT,  &result->mesh[1],
							_voxelAboveRight, _voxelRightBehind, _voxelAboveRightBehind, translate);
					const IndexType v_3_6 = addVertex(reuseVertices, regX, regY + 1, regZ,     voxelLeft, previousSliceVerticesT, &result->mesh[1],
							_voxelAboveRight, voxelBeforeMaterial, _voxelAboveRightBefore, translate);
					vecQuadsT[core::enumVal(FaceNames::PositiveX)][regX].emplace_back(v_0_2, v_3_6, v_2_7, v_1_3);
				}

				// Y [C] BELOW
				if (isQuadNeeded(voxelCurrentMaterial, voxelBelowMaterial, FaceNames::NegativeY)) {
					const Voxel& voxelBelowRightBehind = volumeSampler3.peekVoxel1px1ny1pz();
					const Voxel& voxelBelowRight       = volumeSampler3.peekVoxel1px1ny0pz();
					const Voxel& voxelBelowBehind      = volumeSampler3.peekVoxel0px1ny1pz();

					const VoxelType voxelBelowRightMaterial       = voxelBelowRight.getMaterial();
					const VoxelType voxelBelowBeforeMaterial      = voxelBelowBefore.getMaterial();
					const VoxelType voxelBelowRightBeforeMaterial = voxelBelowRightBefore.getMaterial();
					const VoxelType voxelBelowBehindMaterial      = voxelBelowBehind.getMaterial();
					const VoxelType voxelBelowRightBehindMaterial = voxelBelowRightBehind.getMaterial();

					const IndexType v_0_1 = addVertex(reuseVertices, regX,     regY, regZ,     voxelCurrent, previousSliceVertices, &result->mesh[0],
							voxelBelowBeforeMaterial, voxelBelowLeftMaterial, voxelBelowLeftBeforeMaterial, translate);
					const IndexType v_1_2 = addVertex(reuseVertices, regX + 1, regY, regZ,     voxelCurrent, previousSliceVertices, &result->mesh[0],
							voxelBelowRightMaterial, voxelBelowBeforeMaterial, voxelBelowRightBeforeMaterial, translate);
					const IndexType v_2_3 = addVertex(reuseVertices, regX + 1, regY, regZ + 1, voxelCurrent, currentSliceVertices,  &result->mesh[0],
							voxelBelowBehindMaterial, voxelBelowRightMaterial, voxelBelowRightBehindMaterial, translate);
					const IndexType v_3_4 = addVertex(reuseVertices, regX,     regY, regZ + 1, voxelCurrent, currentSliceVertices,  &result->mesh[0],
							voxelBelowLeftMaterial, voxelBelowBehindMaterial, voxelBelowLeftBehindMaterial, translate);
					vecQuads[core::enumVal(FaceNames::NegativeY)][regY].emplace_back(v_0_1, v_1_2, v_2_3, v_3_4);
				} else if (isTransparentQuadNeeded(voxelCurrentMaterial, voxelBelowMaterial, FaceNames::NegativeY)) {
					const Voxel& voxelBelowRightBehind = volumeSampler3.peekVoxel1px1ny1pz();
					const Voxel& voxelBelowRight       = volumeSampler3.peekVoxel1px1ny0pz();
					const Voxel& voxelBelowBehind      = volumeSampler3.peekVoxel0px1ny1pz();

					const VoxelType voxelBelowRightMaterial       = voxelBelowRight.getMaterial();
					const VoxelType voxelBelowBeforeMaterial      = voxelBelowBefore.getMaterial();
					const VoxelType voxelBelowRightBeforeMaterial = voxelBelowRightBefore.getMaterial();
					const VoxelType voxelBelowBehindMaterial      = voxelBelowBehind.getMaterial();
					const VoxelType voxelBelowRightBehindMaterial = voxelBelowRightBehind.getMaterial();

					const IndexType v_0_1 = addVertex(reuseVertices, regX,     regY, regZ,     voxelCurrent, previousSliceVerticesT, &result->mesh[1],
							voxelBelowBeforeMaterial, voxelBelowLeftMaterial, voxelBelowLeftBeforeMaterial, translate);
					const IndexType v_1_2 = addVertex(reuseVertices, regX + 1, regY, regZ,     voxelCurrent, previousSliceVerticesT, &result->mesh[1],
							voxelBelowRightMaterial, voxelBelowBeforeMaterial, voxelBelowRightBeforeMaterial, translate);
					const IndexType v_2_3 = addVertex(reuseVertices, regX + 1, regY, regZ + 1, voxelCurrent, currentSliceVerticesT,  &result->mesh[1],
							voxelBelowBehindMaterial, voxelBelowRightMaterial, voxelBelowRightBehindMaterial, translate);
					const IndexType v_3_4 = addVertex(reuseVertices, regX,     regY, regZ + 1, voxelCurrent, currentSliceVerticesT,  &result->mesh[1],
							voxelBelowLeftMaterial, voxelBelowBehindMaterial, voxelBelowLeftBehindMaterial, translate);
					vecQuadsT[core::enumVal(FaceNames::NegativeY)][regY].emplace_back(v_0_1, v_1_2, v_2_3, v_3_4);
				}


				// Y [D] ABOVE
				if (isQuadNeeded(voxelBelowMaterial, voxelCurrentMaterial, FaceNames::PositiveY)) {
					const VoxelType _voxelAboveRight       = volumeSampler3.peekVoxel1px0py0pz().getMaterial();
					const VoxelType _voxelAboveBehind      = volumeSampler3.peekVoxel0px0py1pz().getMaterial();
					const VoxelType _voxelAboveRightBehind = volumeSampler3.peekVoxel1px0py1pz().getMaterial();

					const VoxelType _voxelAboveRightBefore = voxelRightBefore.getMaterial();
					const VoxelType _voxelAboveLeftBehind  = voxelLeftBehind.getMaterial();

					const IndexType v_0_5 = addVertex(reuseVertices, regX,     regY, regZ,     voxelBelow, previousSliceVertices, &result->mesh[0],
							voxelBeforeMaterial, voxelLeftMaterial, voxelLeftBeforeMaterial, translate);
					const IndexType v_1_6 = addVertex(reuseVertices, regX + 1, regY, regZ,     voxelBelow, previousSliceVertices, &result->mesh[0],
							_voxelAboveRight, voxelBeforeMaterial, _voxelAboveRightBefore, translate);
					const IndexType v_2_7 = addVertex(reuseVertices, regX + 1, regY, regZ + 1, voxelBelow, currentSliceVertices,  &result->mesh[0],
							_voxelAboveBehind, _voxelAboveRight, _voxelAboveRightBehind, translate);
					const IndexType v_3_8 = addVertex(reuseVertices, regX,     regY, regZ + 1, voxelBelow, currentSliceVertices,  &result->mesh[0],
							voxelLeftMaterial, _voxelAboveBehind, _voxelAboveLeftBehind, translate);
					vecQuads[core::enumVal(FaceNames::PositiveY)][regY].emplace_back(v_0_5, v_3_8, v_2_7, v_1_6);
				} else if (isTransparentQuadNeeded(voxelBelowMaterial, voxelCurrentMaterial, FaceNames::PositiveY)) {
					const VoxelType _voxelAboveRight       = volumeSampler3.peekVoxel1px0py0pz().getMaterial();
					const VoxelType _voxelAboveBehind      = volumeSampler3.peekVoxel0px0py1pz().getMaterial();
					const VoxelType _voxelAboveRightBehind = volumeSampler3.peekVoxel1px0py1pz().getMaterial();

					const VoxelType _voxelAboveRightBefore = voxelRightBefore.getMaterial();
					const VoxelType _voxelAboveLeftBehind  = voxelLeftBehind.getMaterial();

					const IndexType v_0_5 = addVertex(reuseVertices, regX,     regY, regZ,     voxelBelow, previousSliceVerticesT, &result->mesh[1],
							voxelBeforeMaterial, voxelLeftMaterial, voxelLeftBeforeMaterial, translate);
					const IndexType v_1_6 = addVertex(reuseVertices, regX + 1, regY, regZ,     voxelBelow, previousSliceVerticesT, &result->mesh[1],
							_voxelAboveRight, voxelBeforeMaterial, _voxelAboveRightBefore, translate);
					const IndexType v_2_7 = addVertex(reuseVertices, regX + 1, regY, regZ + 1, voxelBelow, currentSliceVerticesT,  &result->mesh[1],
							_voxelAboveBehind, _voxelAboveRight, _voxelAboveRightBehind, translate);
					const IndexType v_3_8 = addVertex(reuseVertices, regX,     regY, regZ + 1, voxelBelow, currentSliceVerticesT,  &result->mesh[1],
							voxelLeftMaterial, _voxelAboveBehind, _voxelAboveLeftBehind, translate);
					vecQuadsT[core::enumVal(FaceNames::PositiveY)][regY].emplace_back(v_0_5, v_3_8, v_2_7, v_1_6);
				}

				// Z [E] BEFORE
				if (isQuadNeeded(voxelCurrentMaterial, voxelBeforeMaterial, FaceNames::NegativeZ)) {
					const VoxelType voxelBelowBeforeMaterial = voxelBelowBefore.getMaterial();
					const VoxelType voxelAboveBeforeMaterial = voxelAboveBefore.getMaterial();
					const VoxelType voxelRightBeforeMaterial = voxelRightBefore.getMaterial();
					const VoxelType voxelAboveRightBeforeMaterial = voxelAboveRightBefore.getMaterial();
					const VoxelType voxelBelowRightBeforeMaterial = voxelBelowRightBefore.getMaterial();

					const IndexType v_0_1 = addVertex(reuseVertices, regX,     regY,     regZ, voxelCurrent, previousSliceVertices, &result->mesh[0],
							voxelBelowBeforeMaterial, voxelLeftBeforeMaterial, voxelBelowLeftBeforeMaterial, translate); //1
					const IndexType v_1_5 = addVertex(reuseVertices, regX,     regY + 1, regZ, voxelCurrent, previousSliceVertices, &result->mesh[0],
							voxelAboveBeforeMaterial, voxelLeftBeforeMaterial, voxelAboveLeftBeforeMaterial, translate); //5
					const IndexType v_2_6 = addVertex(reuseVertices, regX + 1, regY + 1, regZ, voxelCurrent, previousSliceVertices, &result->mesh[0],
							voxelAboveBeforeMaterial, voxelRightBeforeMaterial, voxelAboveRightBeforeMaterial, translate); //6
					const IndexType v_3_2 = addVertex(reuseVertices, regX + 1, regY,     regZ, voxelCurrent, previousSliceVertices, &result->mesh[0],
							voxelBelowBeforeMaterial, voxelRightBeforeMaterial, voxelBelowRightBeforeMaterial, translate); //2
					vecQuads[core::enumVal(FaceNames::NegativeZ)][regZ].emplace_back(v_0_1, v_1_5, v_2_6, v_3_2);
				} else if (isTransparentQuadNeeded(voxelCurrentMaterial, voxelBeforeMaterial, FaceNames::NegativeZ)) {
					const VoxelType voxelBelowBeforeMaterial = voxelBelowBefore.getMaterial();
					const VoxelType voxelAboveBeforeMaterial = voxelAboveBefore.getMaterial();
					const VoxelType voxelRightBeforeMaterial = voxelRightBefore.getMaterial();
					const VoxelType voxelAboveRightBeforeMaterial = voxelAboveRightBefore.getMaterial();
					const VoxelType voxelBelowRightBeforeMaterial = voxelBelowRightBefore.getMaterial();

					const IndexType v_0_1 = addVertex(reuseVertices, regX,     regY,     regZ, voxelCurrent, previousSliceVerticesT, &result->mesh[1],
							voxelBelowBeforeMaterial, voxelLeftBeforeMaterial, voxelBelowLeftBeforeMaterial, translate); //1
					const IndexType v_1_5 = addVertex(reuseVertices, regX,     regY + 1, regZ, voxelCurrent, previousSliceVerticesT, &result->mesh[1],
							voxelAboveBeforeMaterial, voxelLeftBeforeMaterial, voxelAboveLeftBeforeMaterial, translate); //5
					const IndexType v_2_6 = addVertex(reuseVertices, regX + 1, regY + 1, regZ, voxelCurrent, previousSliceVerticesT, &result->mesh[1],
							voxelAboveBeforeMaterial, voxelRightBeforeMaterial, voxelAboveRightBeforeMaterial, translate); //6
					const IndexType v_3_2 = addVertex(reuseVertices, regX + 1, regY,     regZ, voxelCurrent, previousSliceVerticesT, &result->mesh[1],
							voxelBelowBeforeMaterial, voxelRightBeforeMaterial, voxelBelowRightBeforeMaterial, translate); //2
					vecQuadsT[core::enumVal(FaceNames::NegativeZ)][regZ].emplace_back(v_0_1, v_1_5, v_2_6, v_3_2);
				}

				// Z [F] BEHIND
				if (isQuadNeeded(voxelBeforeMaterial, voxelCurrentMaterial, FaceNames::PositiveZ)) {
					const VoxelType _voxelRightBehind      = volumeSampler3.peekVoxel1px0py1pz().getMaterial();
					const VoxelType _voxelAboveBehind      = volumeSampler3.peekVoxel0px1py0pz().getMaterial();
					const VoxelType _voxelAboveRightBehind = volumeSampler3.peekVoxel1px1py0pz().getMaterial();
					const VoxelType _voxelBelowRightBehind = volumeSampler3.peekVoxel1px1ny0pz().getMaterial();

					const IndexType v_0_4 = addVertex(reuseVertices, regX,     regY,     regZ, voxelBefore, previousSliceVertices, &result->mesh[0],
							voxelBelowMaterial, voxelLeftMaterial, voxelBelowLeftMaterial, translate); //4
					const IndexType v_1_8 = addVertex(reuseVertices, regX,     regY + 1, regZ, voxelBefore, previousSliceVertices, &result->mesh[0],
							_voxelAboveBehind, voxelLeftMaterial, voxelAboveLeftMaterial, translate); //8
					const IndexType v_2_7 = addVertex(reuseVertices, regX + 1, regY + 1, regZ, voxelBefore, previousSliceVertices, &result->mesh[0],
							_voxelAboveBehind, _voxelRightBehind, _voxelAboveRightBehind, translate); //7
					const IndexType v_3_3 = addVertex(reuseVertices, regX + 1, regY,     regZ, voxelBefore, previousSliceVertices, &result->mesh[0],
							voxelBelowMaterial, _voxelRightBehind, _voxelBelowRightBehind, translate); //3
					vecQuads[core::enumVal(FaceNames::PositiveZ)][regZ].emplace_back(v_0_4, v_3_3, v_2_7, v_1_8);
				} else if (isTransparentQuadNeeded(voxelBeforeMaterial, voxelCurrentMaterial, FaceNames::PositiveZ)) {
					const VoxelType _voxelRightBehind      = volumeSampler3.peekVoxel1px0py1pz().getMaterial();
					const VoxelType _voxelAboveBehind      = volumeSampler3.peekVoxel0px1py0pz().getMaterial();
					const VoxelType _voxelAboveRightBehind = volumeSampler3.peekVoxel1px1py0pz().getMaterial();
					const VoxelType _voxelBelowRightBehind = volumeSampler3.peekVoxel1px1ny0pz().getMaterial();

					const IndexType v_0_4 = addVertex(reuseVertices, regX,     regY,     regZ, voxelBefore, previousSliceVerticesT, &result->mesh[1],
							voxelBelowMaterial, voxelLeftMaterial, voxelBelowLeftMaterial, translate); //4
					const IndexType v_1_8 = addVertex(reuseVertices, regX,     regY + 1, regZ, voxelBefore, previousSliceVerticesT, &result->mesh[1],
							_voxelAboveBehind, voxelLeftMaterial, voxelAboveLeftMaterial, translate); //8
					const IndexType v_2_7 = addVertex(reuseVertices, regX + 1, regY + 1, regZ, voxelBefore, previousSliceVerticesT, &result->mesh[1],
							_voxelAboveBehind, _voxelRightBehind, _voxelAboveRightBehind, translate); //7
					const IndexType v_3_3 = addVertex(reuseVertices, regX + 1, regY,     regZ, voxelBefore, previousSliceVerticesT, &result->mesh[1],
							voxelBelowMaterial, _voxelRightBehind, _voxelBelowRightBehind, translate); //3
					vecQuadsT[core::enumVal(FaceNames::PositiveZ)][regZ].emplace_back(v_0_4, v_3_3, v_2_7, v_1_8);
				}
				volumeSampler3.movePositiveX();
			}
			volumeSampler2.movePositiveY();
		}
		volumeSampler.movePositiveZ();

		previousSliceVertices.swap(currentSliceVertices);
		previousSliceVerticesT.swap(currentSliceVerticesT);
		currentSliceVertices.clear();
		currentSliceVerticesT.clear();
	}
	}

	{
		core_trace_scoped(GenerateMesh);
		for (QuadListVector& vecListQuads : vecQuads) {
			meshify(&result->mesh[0], mergeQuads, ambientOcclusion, vecListQuads);
		}
		for (QuadListVector& vecListQuads : vecQuadsT) {
			meshify(&result->mesh[1], mergeQuads, ambientOcclusion, vecListQuads);
		}
	}
}
}
