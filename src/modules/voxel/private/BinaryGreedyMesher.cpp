/**
 * @file
 * Binary Greedy Meshing Algorithm
 *
 * This implementation uses binary operations (64-bit masks) combined with greedy meshing
 * to efficiently generate meshes from voxel data. The algorithm processes voxels in chunks,
 * using bitwise operations to identify face visibility and merge adjacent quads.
 *
 * Key concepts:
 * - Binary masking: Uses 64-bit integers as bitmasks to represent voxel occupancy along axes
 * - Face culling: Efficiently identifies which faces are visible using bit shifts
 * - Greedy merging: Extends quads as far as possible in both directions before creating geometry
 * - Ambient occlusion: Optional vertex AO calculation based on neighboring voxels
 */

/*
MIT License

Copyright (c) 2020 Erik Johansson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
 */

#include "BinaryGreedyMesher.h"
#include "app/Async.h"
#include "core/Trace.h"
#include "core/collection/Array.h"
#include "core/collection/Buffer.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelVertex.h"
#include <stdint.h>

namespace voxel {

// CS = chunk size (max 62)
// Limited to 62 because we use 64-bit masks with border padding
static constexpr int CS = 62;

// Padded chunk size - includes 1-voxel border on each side for neighbor sampling
static constexpr int CS_P = CS + 2;
static constexpr int CS_P2 = CS_P * CS_P;
static constexpr int CS_P3 = CS_P * CS_P * CS_P;

/**
 * @brief Converts 3D coordinates to linear index based on axis orientation
 *
 * This function reorders coordinates based on which axis is being processed.
 * The reordering ensures that the "depth" axis (the one being checked for faces)
 * is always in the same position, simplifying the meshing logic.
 *
 * @param axis Which axis is being processed (0=X, 1=Y, 2=Z)
 * @param a First coordinate (interpretation depends on axis)
 * @param b Second coordinate (interpretation depends on axis)
 * @param c Third coordinate (interpretation depends on axis)
 * @return Linear index into the voxel array
 */
CORE_FORCE_INLINE int get_axis_i(const int axis, const int a, const int b, const int c) {
	if (axis == 0)
		return b + (a * CS_P) + (c * CS_P2);
	else if (axis == 1)
		return a + (c * CS_P) + (b * CS_P2);
	return c + (b * CS_P) + (a * CS_P2);
}

/**
 * @brief Checks if a voxel should be considered solid for meshing purposes
 *
 * Template specializations allow different mesh types to have different
 * criteria for what counts as "solid". This affects face culling decisions.
 *
 * @tparam MeshType 0 for opaque geometry, 1 for transparent geometry
 * @param voxel The voxel to check
 * @return true if the voxel is transparent (default behavior)
 */
template<int MeshType>
CORE_FORCE_INLINE bool solid_check(const voxel::Voxel &voxel) {
	return isTransparent(voxel.getMaterial());
}

/**
 * @brief Specialization for opaque mesh type
 *
 * Only considers Generic voxel type as solid for opaque meshes.
 */
template<>
CORE_FORCE_INLINE bool solid_check<0>(const voxel::Voxel &voxel) {
	return voxel.getMaterial() == VoxelType::Generic;
}

// Direction vectors for ambient occlusion sampling
// Includes cardinal directions (first 4) and diagonal directions (last 4)
static const glm::ivec2 ao_dirs[8] = {
	glm::ivec2(-1, 0),	glm::ivec2(0, -1), glm::ivec2(0, 1),  glm::ivec2(1, 0),
	glm::ivec2(-1, -1), glm::ivec2(-1, 1), glm::ivec2(1, -1), glm::ivec2(1, 1),
};

/**
 * @brief Calculates ambient occlusion value for a vertex
 *
 * Uses a simple but effective AO formula based on neighboring voxel occupancy.
 * If both adjacent sides are occupied, the vertex is fully occluded (0).
 * Otherwise, occlusion is based on how many of the three neighbors are occupied.
 *
 * @param side1 Is the first adjacent side occupied? (0 or 1)
 * @param side2 Is the second adjacent side occupied? (0 or 1)
 * @param corner Is the diagonal corner occupied? (0 or 1)
 * @return AO value from 0 (fully occluded) to 3 (no occlusion)
 */
CORE_FORCE_INLINE int vertexAO(uint8_t side1, uint8_t side2, uint8_t corner) {
	return (side1 && side2) ? 0 : (3 - (side1 + side2 + corner));
}

using BinaryMesherInput = core::Buffer<Voxel>;

/**
 * @brief Checks if ambient occlusion values match between two positions
 *
 * For quads to be merged, their ambient occlusion must be consistent.
 * This function compares all 8 AO sampling directions around two positions
 * to ensure they would produce identical AO values.
 *
 * @tparam MeshType Type of mesh being generated
 * @param voxels The voxel data buffer
 * @param axis Current axis being processed
 * @param forward Forward position in current sweep
 * @param right Right position in current sweep
 * @param c Depth position (perpendicular to face)
 * @param forward_offset Offset to compare in forward direction
 * @param right_offset Offset to compare in right direction
 * @return true if AO values match and quads can be merged
 */
template<int MeshType>
CORE_FORCE_INLINE bool compare_ao(const BinaryMesherInput &voxels, int axis, int forward, int right, int c,
								  int forward_offset, int right_offset) {
	for (const auto &ao_dir : ao_dirs) {
		if (solid_check<MeshType>(voxels[get_axis_i(axis, right + ao_dir[0], forward + ao_dir[1], c)]) !=
			solid_check<MeshType>(
				voxels[get_axis_i(axis, right + right_offset + ao_dir[0], forward + forward_offset + ao_dir[1], c)])) {
			return false;
		}
	}
	return true;
}

/**
 * @brief Inserts a quad (two triangles) into the mesh
 *
 * The quad is triangulated based on ambient occlusion to avoid lighting artifacts.
 * Quads are split along the diagonal with the least AO difference.
 *
 * @param mesh The mesh to add triangles to
 * @param v1 First vertex index
 * @param v2 Second vertex index
 * @param v3 Third vertex index
 * @param v4 Fourth vertex index
 * @param flipped If true, flip the triangulation diagonal
 */
CORE_FORCE_INLINE void insert_quad(Mesh &mesh, uint32_t v1, uint32_t v2, uint32_t v3, uint32_t v4, bool flipped) {
	if (flipped) {
		mesh.addTriangle(v1, v2, v3);
		mesh.addTriangle(v3, v4, v1);
	} else {
		mesh.addTriangle(v1, v2, v4);
		mesh.addTriangle(v4, v2, v3);
	}
}

/**
 * @brief Creates or retrieves a vertex for the mesh
 *
 * @param mesh The mesh to add the vertex to
 * @param x X coordinate in chunk space
 * @param y Y coordinate in chunk space
 * @param z Z coordinate in chunk space
 * @param voxel The voxel data for color and flags
 * @param norm Normal vector identifier (face direction)
 * @param ao Ambient occlusion value for this vertex
 * @param translate World space translation offset
 * @return Index of the vertex in the mesh
 */
CORE_FORCE_INLINE uint32_t get_vertex(Mesh &mesh, int32_t x, int32_t y, int32_t z, const voxel::Voxel &voxel,
									  uint32_t norm, uint32_t ao, const glm::ivec3 &translate) {
	VoxelVertex vertex;
	vertex.position = glm::vec3(x - 1 + translate.x, y - 1 + translate.y, z - 1 + translate.z);
	vertex.ambientOcclusion = ao;
	vertex.colorIndex = voxel.getColor();
	vertex.flags = voxel.getFlags();
	vertex.padding = 0u; // Voxel::_unused
	vertex.padding2 = 0u;
	IndexType index = mesh.addVertex(vertex);
	// mesh.setNormal(index, norm);
	return index;
}

// Masks for boundary detection in 64-bit columns
static const uint64_t CULL_MASK = (1ULL << (CS_P - 1));			   // Top boundary bit
static const uint64_t BORDER_MASK = (1ULL | (1ULL << (CS_P - 1))); // Top and bottom boundary bits

/**
 * @brief Prepares chunk data by copying and reordering voxels
 *
 * This function extracts a chunk from the volume and reorganizes it into
 * a format optimized for binary meshing. The voxels are reordered from
 * XYZ layout to YXZ layout, which improves cache coherency during processing.
 *
 * @param map The source voxel volume
 * @param voxels Output buffer for reordered voxel data
 * @param chunkPos Position of the chunk in world space (includes padding)
 */
// TODO: PERF: the binary mesher would be way faster if we would not convert this from xyz to zxy order
void prepareChunk(const voxel::RawVolume &map, BinaryMesherInput &voxels, const glm::ivec3 &chunkPos) {
	core_trace_scoped(PrepareChunks);
	voxel::Region copyRegion(chunkPos, chunkPos + glm::ivec3(CS_P - 1));
	voxel::RawVolume copy(copyRegion);
	copy.copyInto(map, copyRegion);
	const voxel::Voxel *data = copy.voxels();
	voxels.resize(CS_P3);

	// Parallel reorder operation for better cache performance
	auto func = [&voxels, &data](int start, int end) {
		for (int y = start; y < end; y++) {
			const uint32_t yoffset = y * CS_P2;
			const uint32_t vyoffset = y * CS_P;
			for (uint32_t x = 0; x < CS_P; x++) {
				const uint32_t xyoffset = (x * CS_P) + yoffset;
				const uint32_t vxyoffset = x + vyoffset;
				for (uint32_t z = 0; z < CS_P; z++) {
					const int index = z + xyoffset;
					const int vindex = vxyoffset + z * (CS_P2);
					voxels[index] = data[vindex];
				}
			}
		}
	};
	app::for_parallel(0, CS_P, func);
}

/**
 * @brief Extracts mesh geometry using binary greedy meshing algorithm
 *
 * This is the core of the binary greedy meshing algorithm. It works in three phases:
 *
 * Phase 1: Binary Column Generation
 * - Builds 64-bit bitmasks representing voxel occupancy along each axis
 * - Creates separate masks for each of the 6 face directions
 *
 * Phase 2: Face Culling
 * - Uses bit shifts to identify visible faces
 * - A face is visible if there's a solid voxel on one side and air on the other
 * - Culling is done efficiently with bitwise operations: col & ~((col >> 1) | CULL_MASK)
 *
 * Phase 3: Greedy Meshing
 * - Processes each face direction separately
 * - Attempts to merge adjacent faces into larger quads in two passes:
 *   a) Forward pass: extends quads in the "forward" direction
 *   b) Right pass: extends quads in the "right" direction
 * - Only merges faces with matching voxel types and (optionally) matching AO
 *
 * @tparam MeshType 0 for opaque geometry, 1 for transparent geometry
 * @param translate World space offset for vertex positions
 * @param ambientOcclusion Whether to calculate ambient occlusion
 * @param voxels Prepared voxel data buffer
 * @param mesh Output mesh to populate with geometry
 */
template<int MeshType>
void extractBinaryGreedyMeshType(const glm::ivec3 &translate, bool ambientOcclusion, const BinaryMesherInput &voxels,
								 Mesh &mesh) {
	core_trace_scoped(ExtractBinaryGreedyMeshType);

	// Storage for face visibility masks for all 6 directions
	// Each entry is a 64-bit mask representing one "column" of voxels
	alignas(16) core::Array<uint64_t, CS_P2 * 6> col_face_masks({});

	// Temporary storage for accumulated column masks along first axis
	alignas(16) core::Array<uint64_t, CS_P2> a_axis_cols({});

	// === PHASE 1: Build binary columns and cull faces ===

	auto p = voxels.begin();
	for (int a = 0; a < CS_P; a++) {
		// Temporary storage for columns along second axis
		alignas(16) core::Array<uint64_t, CS_P> b_axis_cols({});

		for (int b = 0; b < CS_P; ++b) {
			uint64_t cb = 0; // Column bits for third axis

			// Build column by checking each voxel and setting corresponding bit
			for (int c = 0; c < CS_P; ++c) {
				if (solid_check<MeshType>(*p)) {
					a_axis_cols[b + (c * CS_P)] |= 1ULL << a;
					b_axis_cols[c] |= 1ULL << b;
					cb |= 1ULL << c;
				}
				++p;
			}

			// Cull faces in the third (c) axis direction
			// Face is visible where solid voxel transitions to air
			// Negative direction: shift right and compare
			col_face_masks[a + (b * CS_P) + (4 * CS_P2)] = cb & ~((cb >> 1) | CULL_MASK);
			// Positive direction: shift left and compare
			col_face_masks[a + (b * CS_P) + (5 * CS_P2)] = cb & ~((cb << 1) | 1ULL);
		}

		// Cull faces in the second (b) axis direction
		const int faceIndex = (a * CS_P) + (2 * CS_P2);
		for (int b = 1; b < CS_P - 1; ++b) {
			const uint64_t &col = b_axis_cols[b];
			col_face_masks[faceIndex + b] = col & ~((col >> 1) | CULL_MASK);
			col_face_masks[faceIndex + b + CS_P2] = col & ~((col << 1) | 1ULL);
		}
	}

	// Cull faces in the first (a) axis direction
	for (int a = 1; a < CS_P - 1; a++) {
		const int faceIndex = a * CS_P;
		for (int b = 1; b < CS_P - 1; ++b) {
			const uint64_t &col = a_axis_cols[faceIndex + b];

			col_face_masks[faceIndex + b] = col & ~((col >> 1) | CULL_MASK);
			col_face_masks[faceIndex + b + CS_P2] = col & ~((col << 1) | 1ULL);
		}
	}

	// === PHASE 2 & 3: Greedy meshing for each face direction ===

	for (uint8_t face = 0; face < 6; face++) {
		const int axis = face / 2;					// Which axis this face is perpendicular to
		const int air_dir = face % 2 == 0 ? 1 : -1; // Direction to check for air

		// Tracks how many steps forward each position has been merged
		alignas(16) core::Array<uint64_t, CS_P2> merged_forward({});

		for (int forward = 1; forward < CS_P - 1; forward++) {
			// Tracks which bits are actively being extended rightward
			uint64_t bits_walking_right = 0;
			const int forwardIndex = (forward * CS_P) + (face * CS_P2);

			// Tracks how many steps right each bit position has been merged
			alignas(16) core::Array<uint64_t, CS_P> merged_right({});

			for (int right = 1; right < CS_P - 1; right++) {
				const int rightxCS_P = right * CS_P;

				// Get visibility bits for current, right, and forward positions
				const uint64_t bits_here = col_face_masks[forwardIndex + right] & ~BORDER_MASK;
				const uint64_t bits_right = right >= CS ? 0 : col_face_masks[forwardIndex + right + 1];
				const uint64_t bits_forward = forward >= CS ? 0 : col_face_masks[forwardIndex + right + CS_P];

				// Determine which faces can continue merging
				uint64_t bits_merging_forward = bits_here & bits_forward & ~bits_walking_right;
				const uint64_t bits_merging_right = bits_here & bits_right;

				unsigned long bit_pos;

				// Process faces that can merge forward
				uint64_t copy_front = bits_merging_forward;
				while (copy_front) {
#ifdef _MSC_VER
					_BitScanForward64(&bit_pos, copy_front);
#else
					bit_pos = __builtin_ctzll(copy_front);
#endif

					copy_front &= ~(1ULL << bit_pos);

					// Check if voxel types match and AO is compatible
					if (voxels[get_axis_i(axis, right, forward, bit_pos)].isSame(
							voxels[get_axis_i(axis, right, forward + 1, bit_pos)]) &&
						(!ambientOcclusion ||
						 compare_ao<MeshType>(voxels, axis, forward, right, bit_pos + air_dir, 1, 0))) {
						merged_forward[(right * CS_P) + bit_pos]++;
					} else {
						// Can't merge, remove from merging set
						bits_merging_forward &= ~(1ULL << bit_pos);
					}
				}

				// Process faces that have stopped merging forward
				uint64_t bits_stopped_forward = bits_here & ~bits_merging_forward;
				while (bits_stopped_forward) {
#ifdef _MSC_VER
					_BitScanForward64(&bit_pos, bits_stopped_forward);
#else
					bit_pos = __builtin_ctzll(bits_stopped_forward);
#endif

					bits_stopped_forward &= ~(1ULL << bit_pos);

					const voxel::Voxel &type = voxels[get_axis_i(axis, right, forward, bit_pos)];

					// Try to continue merging rightward
					if ((bits_merging_right & (1ULL << bit_pos)) != 0 &&
						(merged_forward[(right * CS_P) + bit_pos] == merged_forward[(right + 1) * CS_P + bit_pos]) &&
						(type.isSame(voxels[get_axis_i(axis, right + 1, forward, bit_pos)])) &&
						(!ambientOcclusion ||
						 compare_ao<MeshType>(voxels, axis, forward, right, bit_pos + air_dir, 0, 1))) {
						bits_walking_right |= 1ULL << bit_pos;
						merged_right[bit_pos]++;
						merged_forward[rightxCS_P + bit_pos] = 0;
						continue;
					}

					bits_walking_right &= ~(1ULL << bit_pos);

					// Calculate final quad dimensions
					const uint8_t mesh_left = right - merged_right[bit_pos];
					const uint8_t mesh_right = right + 1;
					const uint8_t mesh_front = forward - merged_forward[rightxCS_P + bit_pos];
					const uint8_t mesh_back = forward + 1;
					const uint8_t mesh_up = bit_pos + (face % 2 == 0 ? 1 : 0);

					// Calculate ambient occlusion for all four corners
					uint8_t ao_LB = 3, ao_RB = 3, ao_RF = 3, ao_LF = 3;
					if (ambientOcclusion) {
						const int c = bit_pos + air_dir;

						// Sample adjacent voxels in cardinal directions
						const uint8_t ao_F = solid_check<MeshType>(voxels[get_axis_i(axis, right, forward - 1, c)]);
						const uint8_t ao_B = solid_check<MeshType>(voxels[get_axis_i(axis, right, forward + 1, c)]);
						const uint8_t ao_L = solid_check<MeshType>(voxels[get_axis_i(axis, right - 1, forward, c)]);
						const uint8_t ao_R = solid_check<MeshType>(voxels[get_axis_i(axis, right + 1, forward, c)]);

						// Sample diagonal corners (only if both adjacent sides are empty)
						uint8_t ao_LFC = !ao_L && !ao_F &&
										 solid_check<MeshType>(voxels[get_axis_i(axis, right - 1, forward - 1, c)]);
						uint8_t ao_LBC = !ao_L && !ao_B &&
										 solid_check<MeshType>(voxels[get_axis_i(axis, right - 1, forward + 1, c)]);
						uint8_t ao_RFC = !ao_R && !ao_F &&
										 solid_check<MeshType>(voxels[get_axis_i(axis, right + 1, forward - 1, c)]);
						uint8_t ao_RBC = !ao_R && !ao_B &&
										 solid_check<MeshType>(voxels[get_axis_i(axis, right + 1, forward + 1, c)]);

						// Calculate AO values for each vertex
						ao_LB = vertexAO(ao_L, ao_B, ao_LBC);
						ao_RB = vertexAO(ao_R, ao_B, ao_RBC);
						ao_RF = vertexAO(ao_R, ao_F, ao_RFC);
						ao_LF = vertexAO(ao_L, ao_F, ao_LFC);
					}

					// Reset merge counters
					merged_forward[rightxCS_P + bit_pos] = 0;
					merged_right[bit_pos] = 0;

					// Create vertices with correct orientation based on face direction
					uint32_t v1, v2, v3, v4;
					if (face == 0) {
						v1 = get_vertex(mesh, mesh_left, mesh_up, mesh_back, type, face, ao_LB, translate);
						v2 = get_vertex(mesh, mesh_right, mesh_up, mesh_back, type, face, ao_RB, translate);
						v3 = get_vertex(mesh, mesh_right, mesh_up, mesh_front, type, face, ao_RF, translate);
						v4 = get_vertex(mesh, mesh_left, mesh_up, mesh_front, type, face, ao_LF, translate);
					} else if (face == 1) {
						v1 = get_vertex(mesh, mesh_left, mesh_up, mesh_back, type, face, ao_LB, translate);
						v2 = get_vertex(mesh, mesh_left, mesh_up, mesh_front, type, face, ao_LF, translate);
						v3 = get_vertex(mesh, mesh_right, mesh_up, mesh_front, type, face, ao_RF, translate);
						v4 = get_vertex(mesh, mesh_right, mesh_up, mesh_back, type, face, ao_RB, translate);
					} else if (face == 2) {
						v1 = get_vertex(mesh, mesh_up, mesh_back, mesh_left, type, face, ao_LB, translate);
						v2 = get_vertex(mesh, mesh_up, mesh_back, mesh_right, type, face, ao_RB, translate);
						v3 = get_vertex(mesh, mesh_up, mesh_front, mesh_right, type, face, ao_RF, translate);
						v4 = get_vertex(mesh, mesh_up, mesh_front, mesh_left, type, face, ao_LF, translate);
					} else if (face == 3) {
						v1 = get_vertex(mesh, mesh_up, mesh_back, mesh_left, type, face, ao_LB, translate);
						v2 = get_vertex(mesh, mesh_up, mesh_front, mesh_left, type, face, ao_LF, translate);
						v3 = get_vertex(mesh, mesh_up, mesh_front, mesh_right, type, face, ao_RF, translate);
						v4 = get_vertex(mesh, mesh_up, mesh_back, mesh_right, type, face, ao_RB, translate);
					} else if (face == 4) {
						v1 = get_vertex(mesh, mesh_back, mesh_left, mesh_up, type, face, ao_LB, translate);
						v2 = get_vertex(mesh, mesh_back, mesh_right, mesh_up, type, face, ao_RB, translate);
						v3 = get_vertex(mesh, mesh_front, mesh_right, mesh_up, type, face, ao_RF, translate);
						v4 = get_vertex(mesh, mesh_front, mesh_left, mesh_up, type, face, ao_LF, translate);
					} else {
						v1 = get_vertex(mesh, mesh_back, mesh_left, mesh_up, type, face, ao_LB, translate);
						v2 = get_vertex(mesh, mesh_front, mesh_left, mesh_up, type, face, ao_LF, translate);
						v3 = get_vertex(mesh, mesh_front, mesh_right, mesh_up, type, face, ao_RF, translate);
						v4 = get_vertex(mesh, mesh_back, mesh_right, mesh_up, type, face, ao_RB, translate);
					}

					// Insert quad with optimal triangulation to avoid AO artifacts
					insert_quad(mesh, v1, v2, v3, v4, ao_LB + ao_RF > ao_RB + ao_LF);
				}
			}
		}
	}
}

/**
 * @brief Main entry point for binary greedy mesh extraction
 *
 * Extracts mesh geometry from a voxel volume using the binary greedy meshing algorithm.
 * Generates two separate meshes: one for opaque geometry and one for transparent geometry.
 *
 * The algorithm is highly efficient for large uniform areas, achieving near-optimal
 * quad counts through aggressive greedy merging while maintaining correct ambient occlusion.
 *
 * @param volData Source voxel volume to extract from
 * @param region Region of the volume to process
 * @param result Output chunk mesh (contains two meshes: opaque and transparent)
 * @param translate World space offset for vertex positions
 * @param ambientOcclusion Whether to calculate ambient occlusion
 */
void extractBinaryGreedyMesh(const voxel::RawVolume *volData, const Region &region, ChunkMesh *result,
							 const glm::ivec3 &translate, bool ambientOcclusion) {
	core_trace_scoped(ExtractBinaryGreedyMesh);

	// Set the offset for the chunk mesh
	const glm::ivec3 &offset = region.getLowerCorner();
	result->setOffset(offset);

	// Prepare voxel data with 1-voxel border padding for neighbor access
	// TODO: use the RawVolumeView
	BinaryMesherInput voxels;
	const glm::ivec3 chunkPos = offset - 1;
	prepareChunk(*volData, voxels, chunkPos);

	// Extract opaque geometry (MeshType = 0)
	extractBinaryGreedyMeshType<0>(translate, ambientOcclusion, voxels, result->mesh[0]);

	// Extract transparent geometry (MeshType = 1)
	extractBinaryGreedyMeshType<1>(translate, ambientOcclusion, voxels, result->mesh[1]);
}

} // namespace voxel
