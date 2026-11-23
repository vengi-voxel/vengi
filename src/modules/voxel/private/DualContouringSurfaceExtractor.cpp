/**
 * @file
 */

#include "DualContouringSurfaceExtractor.h"
#include "core/Assert.h"
#include "core/Trace.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

// Based on the polyvox implementation of dual contouring
// https://www.boristhebrave.com/2018/04/15/dual-contouring-tutorial/
// https://people.engr.tamu.edu/schaefer/research/dualsimp_tvcg.pdf

namespace voxel {
namespace {

const glm::ivec3 Axis[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
const glm::ivec3 Corners[8] = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}};
constexpr IndexType InvalidIndex = ~IndexType(0);

struct Cell {
	glm::vec3 position;
	glm::vec3 normal;
	IndexType first = InvalidIndex;
};

struct MaterialVertex {
	IndexType index;
	IndexType next;
	uint8_t color;
	uint8_t flags;
	uint8_t normalId;
	bool transparent;
};

struct Slice {
	core::Buffer<Cell> cells;
	core::DynamicArray<MaterialVertex> vertices;
};

glm::vec3 gradient(const RawVolume &volume, const glm::ivec3 &pos, VoxelType outside) {
	glm::vec3 normal;
	for (int axis = 0; axis < 3; ++axis) {
		const bool lower = volume.voxel(pos - Axis[axis]).getMaterial() > outside;
		const bool upper = volume.voxel(pos + Axis[axis]).getMaterial() > outside;
		normal[axis] = float(lower) - float(upper);
	}
	return normal;
}

void fitCell(const RawVolume &volume, const glm::ivec3 &pos, Cell &cell) {
	VoxelType materials[8];
	bool hasAir = false;
	for (int i = 0; i < 8; ++i) {
		materials[i] = volume.voxel(pos + Corners[i]).getMaterial();
		hasAir |= isAir(materials[i]);
	}
	// At the silhouette, fit the outside of the model. Interior cells fit the opaque/glass interface.
	const VoxelType outside = hasAir ? VoxelType::Air : VoxelType::Transparent;
	glm::dmat3 matrix(0.0);
	glm::dvec3 rhs(0.0);
	glm::dvec3 massPoint(0.0);
	glm::vec3 normalSum(0.0f);
	int crossings = 0;

	for (int i = 0; i < 8; ++i) {
		for (int axis = 0; axis < 3; ++axis) {
			const int bit = 1 << axis;
			if (i & bit) {
				continue;
			}
			const int other = i | bit;
			const bool lowerSolid = materials[i] > outside;
			if (lowerSolid == (materials[other] > outside)) {
				continue;
			}

			glm::vec3 normal =
				gradient(volume, pos + Corners[i], outside) + gradient(volume, pos + Corners[other], outside);
			if (glm::dot(normal, normal) == 0.0f) {
				normal = glm::vec3(Axis[axis]) * (lowerSolid ? 1.0f : -1.0f);
			}
			normal = glm::normalize(normal);
			const glm::dvec3 n(normal);
			const glm::dvec3 point = glm::dvec3(Corners[i]) + glm::dvec3(Axis[axis]) * 0.5;
			matrix += glm::outerProduct(n, n);
			rhs += n * glm::dot(n, point);
			massPoint += point;
			normalSum += normal;
			++crossings;
		}
	}

	core_assert(crossings > 0);
	massPoint /= double(crossings);
	// Regularize the QEF towards the mean crossing to handle rank-deficient cells.
	constexpr double bias = 1e-6;
	const glm::dvec3 fitted = glm::inverse(matrix + glm::dmat3(bias)) * (rhs + bias * massPoint);
	// Keep dual vertices inside their cells so adjacent quads cannot collapse at a cell boundary.
	constexpr float inset = 1.0f / 256.0f;
	cell.position = glm::clamp(glm::vec3(fitted), glm::vec3(inset), glm::vec3(1.0f - inset));
	cell.normal = glm::dot(normalSum, normalSum) > 1e-8f ? glm::normalize(normalSum) : glm::vec3(0.0f);
}

IndexType addVertex(Slice &slice, Cell &cell, Mesh &mesh, const glm::ivec3 &pos, const Voxel &voxel, int axis,
					bool forward, bool interior) {
	const bool flatNormal = interior || cell.normal == glm::vec3(0.0f);
	const uint8_t normalId = flatNormal ? 1 + 2 * axis + int(forward) : 0;
	const uint8_t flags = voxel.getFlags() & FlagOutline;
	const bool transparent = isTransparent(voxel.getMaterial());
	for (IndexType i = cell.first; i != InvalidIndex; i = slice.vertices[i].next) {
		const MaterialVertex &entry = slice.vertices[i];
		if (entry.color == voxel.getColor() && entry.flags == flags && entry.normalId == normalId &&
			entry.transparent == transparent) {
			return entry.index;
		}
	}

	VoxelVertex vertex{};
	vertex.position = glm::vec3(pos) + (cell.position + 0.5f);
	vertex.ambientOcclusion = 3;
	vertex.flags = flags;
	vertex.colorIndex = voxel.getColor();
	const IndexType index = mesh.addVertex(vertex);
	mesh.setNormal(index, flatNormal ? glm::vec3(Axis[axis]) * (forward ? 1.0f : -1.0f) : cell.normal);

	const IndexType entry = IndexType(slice.vertices.size());
	slice.vertices.push_back({index, cell.first, vertex.colorIndex, flags, normalId, transparent});
	cell.first = entry;
	return index;
}

} // namespace

void extractDualContouringMesh(const SurfaceExtractionContext &ctx) {
	core_trace_scoped(ExtractDualContouringMesh);
	if (!ctx.region.isValid()) {
		return;
	}
	const RawVolume &volume = *ctx.volume;
	const glm::ivec3 lower = ctx.region.getLowerCorner();
	const glm::ivec3 upper = ctx.region.getUpperCorner();
	const size_t width = size_t(ctx.region.getWidthInVoxels()) + 1;
	const size_t height = size_t(ctx.region.getHeightInVoxels()) + 1;
	Slice slices[2];
	for (Slice &slice : slices) {
		slice.cells.resize(width * height);
	}
	ctx.mesh.setOffset(lower);

	// The chunk containing the upper endpoint owns the quad. The four incident cells may straddle the chunk.
	for (int z = lower.z; z <= upper.z; ++z) {
		Slice &current = slices[(z - lower.z + 1) % 2];
		current.cells.fill(Cell{});
		current.vertices.clear();
		for (int y = lower.y; y <= upper.y; ++y) {
			for (int x = lower.x; x <= upper.x; ++x) {
				const glm::ivec3 pos(x, y, z);
				const Voxel &upperVoxel = volume.voxel(pos);
				for (int axis = 0; axis < 3; ++axis) {
					const glm::ivec3 base = pos - Axis[axis];
					const Voxel &lowerVoxel = volume.voxel(base);
					if (lowerVoxel.isSameType(upperVoxel)) {
						continue;
					}
					const bool forward = lowerVoxel.getMaterial() > upperVoxel.getMaterial();
					const Voxel &visible = forward ? lowerVoxel : upperVoxel;
					const Voxel &outside = forward ? upperVoxel : lowerVoxel;
					const bool interior = !isAir(outside.getMaterial());
					Mesh &mesh = ctx.mesh.mesh[isTransparent(visible.getMaterial()) ? 1 : 0];

					const glm::ivec3 u = Axis[(axis + 1) % 3];
					const glm::ivec3 v = Axis[(axis + 2) % 3];
					const glm::ivec3 offsets[4] = {{0, 0, 0}, -u, -u - v, -v};
					IndexType indices[4];
					glm::vec3 positions[4];
					for (int i = 0; i < 4; ++i) {
						const glm::ivec3 cellPos = base + offsets[i];
						const glm::ivec3 local = cellPos - lower + 1;
						Slice &slice = slices[local.z % 2];
						Cell &cell = slice.cells[size_t(local.y) * width + size_t(local.x)];
						if (cell.first == InvalidIndex) {
							fitCell(volume, cellPos, cell);
						}
						indices[i] = addVertex(slice, cell, mesh, cellPos - lower + ctx.translate, visible, axis,
											   forward, interior);
						positions[i] = glm::vec3(offsets[i]) + cell.position;
					}

					const auto triangle = [&](int a, int b, int c) {
						mesh.addTriangle(indices[a], indices[forward ? b : c], indices[forward ? c : b]);
					};
					// In projection a quad can be concave, so choose a diagonal that keeps both triangles facing out.
					const glm::vec3 diagonal = positions[2] - positions[0];
					if (glm::cross(positions[1] - positions[0], diagonal)[axis] > 0.0f &&
						glm::cross(diagonal, positions[3] - positions[0])[axis] > 0.0f) {
						triangle(0, 1, 2);
						triangle(0, 2, 3);
					} else {
						triangle(0, 1, 3);
						triangle(1, 2, 3);
					}
				}
			}
		}
	}
}

} // namespace voxel
