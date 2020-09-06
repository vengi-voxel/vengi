/**
 * @file
 */

#pragma once

#include "Mesh.h"
#include "Voxel.h"
#include "VoxelVertex.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/Enum.h"
#include "core/StandardLib.h"
#include "core/NonCopyable.h"
#include "Region.h"
#include "core/Trace.h"
#include "Face.h"
#include <glm/fwd.hpp>
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
	int8_t padding;
};
static_assert(sizeof(VertexData) == 8, "Unexpected size of VertexData");

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
		core_memset(_elements, 0x0, _width * _height * _depth * sizeof(VertexData));
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

extern IndexType addVertex(bool reuseVertices, uint32_t x, uint32_t y, uint32_t z, const Voxel& materialIn, Array& existingVertices,
		Mesh* meshCurrent, const VoxelType face1, const VoxelType face2, const VoxelType corner, const glm::ivec3& offset);

extern void meshify(Mesh* result, bool mergeQuads, QuadListVector& vecListQuads);

/**
 * The CubicSurfaceExtractor creates a mesh in which each voxel appears to be rendered as a cube
 *
 * @par Introduction
 *
 * Games such as Minecraft and Voxatron have a unique graphical style in which each voxel in the world appears to be rendered
 * as a single cube. Actually rendering a cube for each voxel would be very expensive, but in practice the only faces which need
 * to be drawn are those which lie on the boundary between solid and empty voxels. The CubicSurfaceExtractor can be used to create
 * such a mesh from PolyVox volume data. As an example, images from Minecraft and Voxatron are shown below:
 *
 * @image html MinecraftAndVoxatron.jpg
 *
 * Before we get into the specifics of the CubicSurfaceExtractor, it is useful to understand the principles which apply to *all* PolyVox
 * surface extractors and which are described in the Surface Extraction document (ADD LINK). From here on, it is assumed that you
 * are familier with PolyVox regions and how they are used to limit surface extraction to a particular part of the volume. The
 * principles of allowing dynamic terrain are also common to all surface extractors and are described here (ADD LINK).
 *
 * @par Basic Operation
 *
 * At its core, the CubicSurfaceExtractor works by by looking at pairs of adjacent voxels and determining whether a quad should be
 * placed between then. The most simple situation to imagine is a binary volume where every voxel is either solid or empty. In this
 * case a quad should be generated whenever a solid voxel is next to an empty voxel as this represents part of the surface of the
 * solid object. There is no need to generate a quad between two solid voxels (this quad would never be seen as it is inside the
 * object) and there is no need to generate a quad between two empty voxels (there is no object here). PolyVox allows the principle
 * to be extended far beyond such simple binary volumes but they provide a useful starting point for understanding how the algorithm
 * works.
 *
 * As an example, lets consider the part of a volume shown below. We are going to explain the principles in only two dimensions as
 * this makes it much simpler to illustrate, so you will need to mentally extend the process into the third dimension. Hopefully you will
 * find this intuitive. The diagram below shows a small part of a larger volume (as indicated by the voxel coordinates on the axes) which
 * contains only solid and empty voxels represented by solid and hollow circles respectively. The region on which we are running the
 * surface extractor is marked in pink, and for the purpose of this example it corresponds to the whole of the diagram.
 *
 * @image html CubicSurfaceExtractor1.png
 *
 * The output of the surface extractor is the mesh marked in red. As you can see, this forms a closed object which corresponds to the
 * shape of the underlying voxel data.
 *
 * @par Working with Regions
 *
 * So far the behaviour is easy to understand, but let's look at what happens when the extraction is limited to a particular region of
 * the volume. The figure below shows the same data set as the previous figure, but the extraction region (still marked in pink) has
 * been limited to 13 to 16 in x and 47 to 51 in y:
 *
 * @image html CubicSurfaceExtractor2.png
 *
 * As you can see, the extractor continues to generate a number of quads as indicated by the solid red lines. However, you can also see
 * that the shape is no longer closed. This is because the solid voxels actually extend outside the region which is being processed, and
 * so the extractor does not encounter a boundary between solid and empty voxels. Although this may initially appear problematic, the
 * hole in the mesh does not actually matter because it will be hidden by the mesh corresponding to the region adjacent to it (see next
 * diagram).
 *
 * More interestingly, the diagram also contains a couple of dotted red lines lying on the bottom and right hand side of the extracted
 * region. These are present to illustrate a common point of confusion, which is that *no quads are generated at this position even though
 * it is a boundary between solid and empty voxels*. This is indeed somewhat counter intuitive but there is a rational reasaoning behind
 * it.
 * If you consider the dashed line on the righthand side of the extracted region, then it is clear that this lies on a boundary between
 * solid and empty voxels and so we do need to create quads here. But what is not so clear is whether these quads should be assigned to
 * the mesh which corresponds to the region in pink, or whether they should be assigned to the region to the right of it which is marked
 * in blue in the diagram below:
 *
 * @image html CubicSurfaceExtractor3.png
 *
 * We could choose to add the quads to *both* regions, but this can cause confusion when one of the region is modified (causing the face
 * to disappear or a new one to be created) as *both* regions need to have their mesh regenerated to correctly represent the new state of
 * the volume data. Such pairs of coplanar quads can also cause problems with physics engines, and may prevent transparent voxels from
 * rendering correctly. Therefore we choose to instead only add the quad to one of the the regions and we always choose the one with the
 * greater coordinate value in the direction in which they differ. In the above example the regions differ by the 'x' component of their
 * position, and so the quad is added to the region with the greater 'x' value (the one marked in blue).
 *
 * One of the practical implications of this is that when you modify a voxel *you may have to re-extract the mesh for regions other than
 * region which actually contains the voxel you modified.* This happens when the voxel lies on the upper x,y or z face of a region.
 * Assuming that you have some management code which can mark a region as needing re-extraction when a voxel changes, you should probably
 * extend this to mark the regions of neighbouring voxels as invalid (this will have no effect when the voxel is well within a region,
 * but will mark the neighbouring region as needing an update if the voxel lies on a region face).
 *
 * Another scenario which sometimes results in confusion is when you wish to extract a region which corresponds to the whole volume,
 * particularly when solid voxels extend right to the edge of the volume.
 *
 * This version of the function performs the extraction into a user-provided mesh rather than allocating a mesh automatically.
 * There are a few reasons why this might be useful to more advanced users:
 *
 * @li It leaves the user in control of memory allocation and would allow them to implement e.g. a mesh pooling system.
 * @li The user-provided mesh could have a different index type (e.g. 16-bit indices) to reduce memory usage.
 * @li The user could provide a custom mesh class, e.g a thin wrapper around an openGL VBO to allow direct writing into this structure.
 */
template<typename VolumeType, typename IsQuadNeeded>
void extractCubicMesh(VolumeType* volData, const Region& region, Mesh* result, IsQuadNeeded isQuadNeeded, const glm::ivec3& translate, bool mergeQuads = true, bool reuseVertices = true) {
	core_trace_scoped(ExtractCubicMesh);

	result->clear();
	const glm::ivec3& offset = region.getLowerCorner();
	const glm::ivec3& upper = region.getUpperCorner();
	result->setOffset(offset);

	// Used to avoid creating duplicate vertices.
	const int widthInCells = upper.x - offset.x;
	const int heightInCells = upper.y - offset.y;
	Array previousSliceVertices(widthInCells + 2, heightInCells + 2, MaxVerticesPerPosition);
	Array currentSliceVertices(widthInCells + 2, heightInCells + 2, MaxVerticesPerPosition);

	// During extraction we create a number of different lists of quads. All the
	// quads in a given list are in the same plane and facing in the same direction.
	QuadListVector vecQuads[core::enumVal(FaceNames::Max)];

	const int xSize = upper.x - offset.x + 2;
	const int ySize = upper.y - offset.y + 2;
	const int zSize = upper.z - offset.z + 2;
	vecQuads[core::enumVal(FaceNames::NegativeX)].resize(xSize);
	vecQuads[core::enumVal(FaceNames::PositiveX)].resize(xSize);

	vecQuads[core::enumVal(FaceNames::NegativeY)].resize(ySize);
	vecQuads[core::enumVal(FaceNames::PositiveY)].resize(ySize);

	vecQuads[core::enumVal(FaceNames::NegativeZ)].resize(zSize);
	vecQuads[core::enumVal(FaceNames::PositiveZ)].resize(zSize);

	typename VolumeType::Sampler volumeSampler(volData);

	{
	core_trace_scoped(QuadGeneration);
	for (int32_t z = offset.z; z <= upper.z; ++z) {
		const uint32_t regZ = z - offset.z;
		for (int32_t x = offset.x; x <= upper.x; ++x) {
			const uint32_t regX = x - offset.x;
			volumeSampler.setPosition(x, offset.y, z);
			for (int32_t y = offset.y; y <= upper.y; ++y) {
				const uint32_t regY = y - offset.y;

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

				const Voxel& voxelCurrent          = volumeSampler.voxel();
				const Voxel& voxelLeft             = volumeSampler.peekVoxel1nx0py0pz();
				const Voxel& voxelBefore           = volumeSampler.peekVoxel0px0py1nz();
				const Voxel& voxelLeftBefore       = volumeSampler.peekVoxel1nx0py1nz();
				const Voxel& voxelRightBefore      = volumeSampler.peekVoxel1px0py1nz();
				const Voxel& voxelLeftBehind       = volumeSampler.peekVoxel1nx0py1pz();

				const Voxel& voxelAboveLeft        = volumeSampler.peekVoxel1nx1py0pz();
				const Voxel& voxelAboveBefore      = volumeSampler.peekVoxel0px1py1nz();
				const Voxel& voxelAboveLeftBefore  = volumeSampler.peekVoxel1nx1py1nz();
				const Voxel& voxelAboveRightBefore = volumeSampler.peekVoxel1px1py1nz();
				const Voxel& voxelAboveLeftBehind  = volumeSampler.peekVoxel1nx1py1pz();

				const Voxel& voxelBelow            = volumeSampler.peekVoxel0px1ny0pz();
				const Voxel& voxelBelowLeft        = volumeSampler.peekVoxel1nx1ny0pz();
				const Voxel& voxelBelowBefore      = volumeSampler.peekVoxel0px1ny1nz();
				const Voxel& voxelBelowLeftBefore  = volumeSampler.peekVoxel1nx1ny1nz();
				const Voxel& voxelBelowRightBefore = volumeSampler.peekVoxel1px1ny1nz();
				const Voxel& voxelBelowLeftBehind  = volumeSampler.peekVoxel1nx1ny1pz();

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
					const IndexType v_0_1 = addVertex(reuseVertices, regX, regY,     regZ,     voxelCurrent, previousSliceVertices, result,
							voxelLeftBeforeMaterial, voxelBelowLeftMaterial, voxelBelowLeftBeforeMaterial, translate);
					const IndexType v_1_4 = addVertex(reuseVertices, regX, regY,     regZ + 1, voxelCurrent, currentSliceVertices,  result,
							voxelBelowLeftMaterial, voxelLeftBehindMaterial, voxelBelowLeftBehindMaterial, translate);
					const IndexType v_2_8 = addVertex(reuseVertices, regX, regY + 1, regZ + 1, voxelCurrent, currentSliceVertices,  result,
							voxelLeftBehindMaterial, voxelAboveLeftMaterial, voxelAboveLeftBehindMaterial, translate);
					const IndexType v_3_5 = addVertex(reuseVertices, regX, regY + 1, regZ,     voxelCurrent, previousSliceVertices, result,
							voxelAboveLeftMaterial, voxelLeftBeforeMaterial, voxelAboveLeftBeforeMaterial, translate);
					vecQuads[core::enumVal(FaceNames::NegativeX)][regX].emplace_back(v_0_1, v_1_4, v_2_8, v_3_5);
				}

				// X [B] RIGHT
				if (isQuadNeeded(voxelLeftMaterial, voxelCurrentMaterial, FaceNames::PositiveX)) {
					const VoxelType _voxelRightBehind      = volumeSampler.peekVoxel0px0py1pz().getMaterial();
					const VoxelType _voxelAboveRight       = volumeSampler.peekVoxel0px1py0pz().getMaterial();
					const VoxelType _voxelAboveRightBehind = volumeSampler.peekVoxel0px1py1pz().getMaterial();
					const VoxelType _voxelBelowRightBehind = volumeSampler.peekVoxel0px1ny1pz().getMaterial();

					const VoxelType _voxelAboveRightBefore = voxelAboveBefore.getMaterial();
					const VoxelType _voxelBelowRightBefore = voxelBelowBefore.getMaterial();

					const IndexType v_0_2 = addVertex(reuseVertices, regX, regY,     regZ,     voxelLeft, previousSliceVertices, result,
							voxelBelowMaterial, voxelBeforeMaterial, _voxelBelowRightBefore, translate);
					const IndexType v_1_3 = addVertex(reuseVertices, regX, regY,     regZ + 1, voxelLeft, currentSliceVertices,  result,
							voxelBelowMaterial, _voxelRightBehind, _voxelBelowRightBehind, translate);
					const IndexType v_2_7 = addVertex(reuseVertices, regX, regY + 1, regZ + 1, voxelLeft, currentSliceVertices,  result,
							_voxelAboveRight, _voxelRightBehind, _voxelAboveRightBehind, translate);
					const IndexType v_3_6 = addVertex(reuseVertices, regX, regY + 1, regZ,     voxelLeft, previousSliceVertices, result,
							_voxelAboveRight, voxelBeforeMaterial, _voxelAboveRightBefore, translate);
					vecQuads[core::enumVal(FaceNames::PositiveX)][regX].emplace_back(v_0_2, v_3_6, v_2_7, v_1_3);
				}

				// Y [C] BELOW
				if (isQuadNeeded(voxelCurrentMaterial, voxelBelowMaterial, FaceNames::NegativeY)) {
					const Voxel& voxelBelowRightBehind = volumeSampler.peekVoxel1px1ny1pz();
					const Voxel& voxelBelowRight       = volumeSampler.peekVoxel1px1ny0pz();
					const Voxel& voxelBelowBehind      = volumeSampler.peekVoxel0px1ny1pz();

					const VoxelType voxelBelowRightMaterial       = voxelBelowRight.getMaterial();
					const VoxelType voxelBelowBeforeMaterial      = voxelBelowBefore.getMaterial();
					const VoxelType voxelBelowRightBeforeMaterial = voxelBelowRightBefore.getMaterial();
					const VoxelType voxelBelowBehindMaterial      = voxelBelowBehind.getMaterial();
					const VoxelType voxelBelowRightBehindMaterial = voxelBelowRightBehind.getMaterial();

					const IndexType v_0_1 = addVertex(reuseVertices, regX,     regY, regZ,     voxelCurrent, previousSliceVertices, result,
							voxelBelowBeforeMaterial, voxelBelowLeftMaterial, voxelBelowLeftBeforeMaterial, translate);
					const IndexType v_1_2 = addVertex(reuseVertices, regX + 1, regY, regZ,     voxelCurrent, previousSliceVertices, result,
							voxelBelowRightMaterial, voxelBelowBeforeMaterial, voxelBelowRightBeforeMaterial, translate);
					const IndexType v_2_3 = addVertex(reuseVertices, regX + 1, regY, regZ + 1, voxelCurrent, currentSliceVertices,  result,
							voxelBelowBehindMaterial, voxelBelowRightMaterial, voxelBelowRightBehindMaterial, translate);
					const IndexType v_3_4 = addVertex(reuseVertices, regX,     regY, regZ + 1, voxelCurrent, currentSliceVertices,  result,
							voxelBelowLeftMaterial, voxelBelowBehindMaterial, voxelBelowLeftBehindMaterial, translate);
					vecQuads[core::enumVal(FaceNames::NegativeY)][regY].emplace_back(v_0_1, v_1_2, v_2_3, v_3_4);
				}

				// Y [D] ABOVE
				if (isQuadNeeded(voxelBelowMaterial, voxelCurrentMaterial, FaceNames::PositiveY)) {
					const VoxelType _voxelAboveRight       = volumeSampler.peekVoxel1px0py0pz().getMaterial();
					const VoxelType _voxelAboveBehind      = volumeSampler.peekVoxel0px0py1pz().getMaterial();
					const VoxelType _voxelAboveRightBehind = volumeSampler.peekVoxel1px0py1pz().getMaterial();

					const VoxelType _voxelAboveRightBefore = voxelRightBefore.getMaterial();
					const VoxelType _voxelAboveLeftBehind  = voxelLeftBehind.getMaterial();

					const IndexType v_0_5 = addVertex(reuseVertices, regX,     regY, regZ,     voxelBelow, previousSliceVertices, result,
							voxelBeforeMaterial, voxelLeftMaterial, voxelLeftBeforeMaterial, translate);
					const IndexType v_1_6 = addVertex(reuseVertices, regX + 1, regY, regZ,     voxelBelow, previousSliceVertices, result,
							_voxelAboveRight, voxelBeforeMaterial, _voxelAboveRightBefore, translate);
					const IndexType v_2_7 = addVertex(reuseVertices, regX + 1, regY, regZ + 1, voxelBelow, currentSliceVertices,  result,
							_voxelAboveBehind, _voxelAboveRight, _voxelAboveRightBehind, translate);
					const IndexType v_3_8 = addVertex(reuseVertices, regX,     regY, regZ + 1, voxelBelow, currentSliceVertices,  result,
							voxelLeftMaterial, _voxelAboveBehind, _voxelAboveLeftBehind, translate);
					vecQuads[core::enumVal(FaceNames::PositiveY)][regY].emplace_back(v_0_5, v_3_8, v_2_7, v_1_6);
				}

				// Z [E] BEFORE
				if (isQuadNeeded(voxelCurrentMaterial, voxelBeforeMaterial, FaceNames::NegativeZ)) {
					const VoxelType voxelBelowBeforeMaterial = voxelBelowBefore.getMaterial();
					const VoxelType voxelAboveBeforeMaterial = voxelAboveBefore.getMaterial();
					const VoxelType voxelRightBeforeMaterial = voxelRightBefore.getMaterial();
					const VoxelType voxelAboveRightBeforeMaterial = voxelAboveRightBefore.getMaterial();
					const VoxelType voxelBelowRightBeforeMaterial = voxelBelowRightBefore.getMaterial();

					const IndexType v_0_1 = addVertex(reuseVertices, regX,     regY,     regZ, voxelCurrent, previousSliceVertices, result,
							voxelBelowBeforeMaterial, voxelLeftBeforeMaterial, voxelBelowLeftBeforeMaterial, translate); //1
					const IndexType v_1_5 = addVertex(reuseVertices, regX,     regY + 1, regZ, voxelCurrent, previousSliceVertices, result,
							voxelAboveBeforeMaterial, voxelLeftBeforeMaterial, voxelAboveLeftBeforeMaterial, translate); //5
					const IndexType v_2_6 = addVertex(reuseVertices, regX + 1, regY + 1, regZ, voxelCurrent, previousSliceVertices, result,
							voxelAboveBeforeMaterial, voxelRightBeforeMaterial, voxelAboveRightBeforeMaterial, translate); //6
					const IndexType v_3_2 = addVertex(reuseVertices, regX + 1, regY,     regZ, voxelCurrent, previousSliceVertices, result,
							voxelBelowBeforeMaterial, voxelRightBeforeMaterial, voxelBelowRightBeforeMaterial, translate); //2
					vecQuads[core::enumVal(FaceNames::NegativeZ)][regZ].emplace_back(v_0_1, v_1_5, v_2_6, v_3_2);
				}

				// Z [F] BEHIND
				if (isQuadNeeded(voxelBeforeMaterial, voxelCurrentMaterial, FaceNames::PositiveZ)) {
					const VoxelType _voxelRightBehind      = volumeSampler.peekVoxel1px0py1pz().getMaterial();
					const VoxelType _voxelAboveBehind      = volumeSampler.peekVoxel0px1py0pz().getMaterial();
					const VoxelType _voxelAboveRightBehind = volumeSampler.peekVoxel1px1py0pz().getMaterial();
					const VoxelType _voxelBelowRightBehind = volumeSampler.peekVoxel1px1ny0pz().getMaterial();

					const IndexType v_0_4 = addVertex(reuseVertices, regX,     regY,     regZ, voxelBefore, previousSliceVertices, result,
							voxelBelowMaterial, voxelLeftMaterial, voxelBelowLeftMaterial, translate); //4
					const IndexType v_1_8 = addVertex(reuseVertices, regX,     regY + 1, regZ, voxelBefore, previousSliceVertices, result,
							_voxelAboveBehind, voxelLeftMaterial, voxelAboveLeftMaterial, translate); //8
					const IndexType v_2_7 = addVertex(reuseVertices, regX + 1, regY + 1, regZ, voxelBefore, previousSliceVertices, result,
							_voxelAboveBehind, _voxelRightBehind, _voxelAboveRightBehind, translate); //7
					const IndexType v_3_3 = addVertex(reuseVertices, regX + 1, regY,     regZ, voxelBefore, previousSliceVertices, result,
							voxelBelowMaterial, _voxelRightBehind, _voxelBelowRightBehind, translate); //3
					vecQuads[core::enumVal(FaceNames::PositiveZ)][regZ].emplace_back(v_0_4, v_3_3, v_2_7, v_1_8);
				}

				if (core_likely(y != upper.y)) {
					volumeSampler.movePositiveY();
				}
			}
		}

		previousSliceVertices.swap(currentSliceVertices);
		currentSliceVertices.clear();
	}
	}

	{
		core_trace_scoped(GenerateMesh);
		for (QuadListVector& vecListQuads : vecQuads) {
			meshify(result, mergeQuads, vecListQuads);
		}
	}

	result->removeUnusedVertices();
	result->compressIndices();
}

}

#undef BUFFERED_SAMPLER
