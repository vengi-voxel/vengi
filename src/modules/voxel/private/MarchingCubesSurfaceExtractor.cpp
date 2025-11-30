/**
 * @file
 */

#include "MarchingCubesSurfaceExtractor.h"
#include "color/Color.h"
#include "core/Common.h"
#include "core/Trace.h"
#include "core/collection/Array2DView.h"
#include "MarchingCubesTables.h"
#include "math/Axis.h"
#include "voxel/ChunkMesh.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/VoxelVertex.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/norm.hpp>
#include <glm/vec3.hpp>
#include <stdint.h>

namespace voxel {

static const float MarchingCubeMaxDensity = 255.0f;
static const float DensityThreshold = MarchingCubeMaxDensity / 2.0f; // isolevel

static inline float convertToDensity(const Voxel &voxel) {
	return isAir(voxel.getMaterial()) ? 0.0f : MarchingCubeMaxDensity;
}

static inline Voxel blendMaterials(const palette::Palette &palette, const Voxel &v1, const Voxel &v2, float val) {
	if (isAir(v1.getMaterial())) {
		return v2;
	}
	if (isAir(v2.getMaterial())) {
		return v1;
	}
	const core::RGBA c1 = palette.color(v1.getColor());
	const core::RGBA c2 = palette.color(v2.getColor());
	const core::RGBA blended = core::RGBA::mix(c1, c2, val);
	const uint8_t palIdx = palette.getClosestMatch(blended);
	return createVoxel(palette, palIdx);
}

// Gradient estimation
static glm::vec3 computeCentralDifferenceGradient(const RawVolume::Sampler &volIter) {
	const float voxel1nx = convertToDensity(volIter.peekVoxel1nx0py0pz());
	const float voxel1px = convertToDensity(volIter.peekVoxel1px0py0pz());

	const float voxel1ny = convertToDensity(volIter.peekVoxel0px1ny0pz());
	const float voxel1py = convertToDensity(volIter.peekVoxel0px1py0pz());

	const float voxel1nz = convertToDensity(volIter.peekVoxel0px0py1nz());
	const float voxel1pz = convertToDensity(volIter.peekVoxel0px0py1pz());

	return glm::vec3(voxel1nx - voxel1px, voxel1ny - voxel1py, voxel1nz - voxel1pz);
}

static void generateVertex(math::Axis axis, const palette::Palette &palette, RawVolume::Sampler &sampler,
				   ChunkMesh *result, core::Array2DView<glm::ivec3> &indicesView,
				   const Voxel &v111, const glm::vec3 &n111, float v111Density, int x, int y) {
	sampler.moveNegative(axis);
	const Voxel v110 = sampler.voxel();
	const float v110Density = convertToDensity(v110);
	const float interpolate = (DensityThreshold - v110Density) / (v111Density - v110Density);

	// Compute the normal
	const glm::vec3 n110 = computeCentralDifferenceGradient(sampler);
	glm::vec3 normal = (n111 * interpolate) + (n110 * (1 - interpolate));

	// The gradient for a voxel can be zero (e.g. solid voxel surrounded by empty ones) and so
	// the interpolated normal can also be zero (e.g. a grid of alternating solid and empty voxels).
	const float normLen = glm::length2(normal);
	if (normLen > 0.000001f) {
		normal *= glm::inversesqrt(normLen);
	}

	const Voxel blendedVoxel = blendMaterials(palette, v110, v111, interpolate);

	const int idx = math::getIndexForAxis(axis);
	VoxelVertex surfaceVertex;
	surfaceVertex.position = sampler.position();
	surfaceVertex.position[idx] += interpolate;
	surfaceVertex.colorIndex = blendedVoxel.getColor();
	surfaceVertex.normalIndex = 255;
	surfaceVertex.padding2 = 0;
	surfaceVertex.info = 0;
	surfaceVertex.flags = blendedVoxel.getFlags();

	const IndexType lastVertexIndex = result->mesh[0].addVertex(surfaceVertex);
	result->mesh[0].setNormal(lastVertexIndex, normal);
	indicesView.get(x, y)[idx] = (int)lastVertexIndex;

	sampler.movePositive(axis);
}

void extractMarchingCubesMesh(const RawVolume *volume, const palette::Palette &palette, const Region &ctxRegion, ChunkMesh *result) {
	core_trace_scoped(ExtractMarchingCubesMesh);
	voxel::Region region = ctxRegion;
	region.shrink(-1);

	// Store some commonly used values for performance and convenience
	const int32_t w = region.getWidthInVoxels();
	const int32_t h = region.getHeightInVoxels();
	const int32_t d = region.getDepthInVoxels();

	// A naive implementation of Marching Cubes might sample the eight corner voxels of every cell to determine the cell
	// index. However, when processing the cells sequentially we can observe that many of the voxels are shared with
	// previous adjacent cells, and so we can obtain these by careful bit-shifting. These variables keep track of
	// previous cells for this purpose.
	uint8_t previousCellIndex = 0;
	core::Buffer<uint8_t> previousRowCellIndices(w);
	core::Buffer<uint8_t> previousSliceCellIndicesBuf((size_t)(w * h));
	core::Array2DView<uint8_t> previousSliceCellIndicesView(previousSliceCellIndicesBuf.data(), w, h);

	// A given vertex may be shared by multiple triangles, so we need to keep track of the indices into the vertex
	// array.
	core::Buffer<glm::ivec3> indicesBuf((size_t)(w * h));
	core::Buffer<glm::ivec3> previousIndicesBuf((size_t)(w * h));

	// A sampler pointing at the beginning of the region, which gets incremented to always point at the beginning of a
	// slice.
	RawVolume::Sampler startOfSlice(volume);
	startOfSlice.setPosition(region.getLowerCorner());

	for (int32_t z = 0; z < d; z++) {
		// A sampler pointing at the beginning of the slice, which gets incremented to always point at the beginning of
		// a row.
		RawVolume::Sampler startOfRow = startOfSlice;

		core::Array2DView<glm::ivec3> indicesView(indicesBuf.data(), w, h);
		core::Array2DView<glm::ivec3> previousIndicesView(previousIndicesBuf.data(), w, h);

		for (int32_t y = 0; y < h; y++) {
			// Copying a sampler which is already pointing at the correct location seems (slightly) faster than
			// calling setPosition(). Therefore we make use of 'startOfRow' and 'startOfSlice' to reset the sampler.
			RawVolume::Sampler sampler = startOfRow;

			for (int32_t x = 0; x < w; x++) {
				// Note: In many cases the provided region will be (mostly) empty which means mesh vertices/indices
				// are not generated and the only thing that is done for each cell is the computation of "cellIndex".
				// It appears that retrieving the voxel value is not so expensive and that it is the bitwise combining
				// which actually carries the cost.
				//
				// If we really need to speed this up more then it may be possible to pack 4 8-bit cell indices into
				// a single 32-bit value and then perform the bitwise logic on all four of them at the same time.
				// However, this complicates the code and there would still be the cost of packing/unpacking so it's
				// not clear if there is really a benefit. It's something to consider in the future.

				// Each bit of the cell index specifies whether a given corner of the cell is above or below the
				// threshold.
				uint8_t cellIndex = 0;

				// Four bits of our cube index are obtained by looking at the cube index for
				// the previous slice and copying four of those bits into their new positions.
				uint8_t previousCellIndexZ = previousSliceCellIndicesView.get(x, y);
				previousCellIndexZ >>= 4;
				cellIndex |= previousCellIndexZ;

				// Two bits of our cube index are obtained by looking at the cube index for
				// the previous row and copying two of those bits into their new positions.
				uint8_t previousCellIndexY = previousRowCellIndices[x];
				previousCellIndexY &= 204; // 204 = 128+64+8+4
				previousCellIndexY >>= 2;
				cellIndex |= previousCellIndexY;

				// One bit of our cube index are obtained by looking at the cube index for
				// the previous cell and copying one of those bits into it's new position.
				uint8_t previousCellIndexX = previousCellIndex;
				previousCellIndexX &= 170; // 170 = 128+32+8+2
				previousCellIndexX >>= 1;
				cellIndex |= previousCellIndexX;

				// The last bit of our cube index is obtained by looking
				// at the relevant voxel and comparing it to the threshold
				const Voxel v111 = sampler.voxel();
				if (convertToDensity(v111) < DensityThreshold) {
					cellIndex |= 128;
				}

				// The current value becomes the previous value, ready for the next iteration.
				previousCellIndex = cellIndex;
				previousRowCellIndices[x] = cellIndex;
				previousSliceCellIndicesView.set(x, y, cellIndex);

				// 12 bits of edge determine whether a vertex is placed on each of the 12 edges of the cell.
				const uint16_t edge = edgeTable[cellIndex];

				// Test whether any vertices and indices should be generated for the current cell (i.e. it is occupied).
				// Performance note: This condition is usually false because most cells in a volume are completely above
				// or below the threshold and hence unoccupied. However, even when it is always false (testing on an
				// empty volume) it still incurs significant overhead, probably because the code is large and bloats the
				// for loop which contains it. On my empty volume test case the code as given runs in 34ms, but if I
				// replace the condition with 'false' it runs in 24ms and gives the same output (i.e. none).
				if (core_unlikely(edge != 0u)) {
					const float v111Density = convertToDensity(v111);

					// Performance note: Computing normals is one of the bottlenecks in the mesh generation process. The
					// central difference approach actually samples the same voxel more than once as we call it on two
					// adjacent voxels. Perhaps we could expand this and eliminate duplicates in the future.
					// Alternatively, we could compute vertex normals from adjacent face normals instead of via central
					// differencing, but not for vertices on the edge of the region (as this causes visual
					// discontinuities).
					const glm::vec3 n111 = computeCentralDifferenceGradient(sampler);

					/* Find the vertices where the surface intersects the cube */
					if ((edge & 64) && x > 0) {
						generateVertex(math::Axis::X, palette, sampler, result, indicesView, v111, n111, v111Density, x, y);
					}
					if ((edge & 32) && y > 0) {
						generateVertex(math::Axis::Y, palette, sampler, result, indicesView, v111, n111, v111Density, x, y);
					}
					if ((edge & 1024) && z > 0) {
						generateVertex(math::Axis::Z, palette, sampler, result, indicesView, v111, n111, v111Density, x, y);
					}

					// Now output the indices. For the first row, column or slice there aren't
					// any (the region size in cells is one less than the region size in voxels)
					if (x != 0 && y != 0 && z != 0) {
						int32_t indlist[12] { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

						/* Find the vertices where the surface intersects the cube */
						if (edge & 1) {
							indlist[0] = previousIndicesView.get(x, y - 1).x;
						}
						if (edge & 2) {
							indlist[1] = previousIndicesView.get(x, y).y;
						}
						if (edge & 4) {
							indlist[2] = previousIndicesView.get(x, y).x;
						}
						if (edge & 8) {
							indlist[3] = previousIndicesView.get(x - 1, y).y;
						}
						if (edge & 16) {
							indlist[4] = indicesView.get(x, y - 1).x;
						}
						if (edge & 32) {
							indlist[5] = indicesView.get(x, y).y;
						}
						if (edge & 64) {
							indlist[6] = indicesView.get(x, y).x;
						}
						if (edge & 128) {
							indlist[7] = indicesView.get(x - 1, y).y;
						}
						if (edge & 256) {
							indlist[8] = indicesView.get(x - 1, y - 1).z;
						}
						if (edge & 512) {
							indlist[9] = indicesView.get(x, y - 1).z;
						}
						if (edge & 1024) {
							indlist[10] = indicesView.get(x, y).z;
						}
						if (edge & 2048) {
							indlist[11] = indicesView.get(x - 1, y).z;
						}

						for (int i = 0; triTable[cellIndex][i] != -1; i += 3) {
							const int32_t ind0 = indlist[triTable[cellIndex][i + 0]];
							const int32_t ind1 = indlist[triTable[cellIndex][i + 1]];
							const int32_t ind2 = indlist[triTable[cellIndex][i + 2]];

							if (ind0 != -1 && ind1 != -1 && ind2 != -1) {
								result->mesh[0].addTriangle(ind0, ind1, ind2);
							}
						}
					}
				}
				sampler.movePositiveX();
			}
			startOfRow.movePositiveY();
		}
		startOfSlice.movePositiveZ();

		core::exchange(indicesBuf, previousIndicesBuf);
	}
	result->setOffset(region.getLowerCorner());
}

} // namespace voxel
