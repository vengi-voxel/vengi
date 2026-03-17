/**
 * @file
 */

#include "SmoothMeshExtractor.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "voxel/ChunkMesh.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelVertex.h"
#include "voxel/private/MarchingCubesTables.h"
#include <glm/geometric.hpp>
// std::unordered_map is used here because core::DynamicMap requires a hash specialization
// for uint64_t which doesn't exist, and this is a temporary allocation freed after MC.
#include <unordered_map>

namespace voxelutil {

// Polyvox bit-to-corner mapping (bits 2<->3 and 6<->7 swapped vs standard)
static const glm::ivec3 cornerOffsets[8] = {
	{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0},
	{0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}
};

static const int edgeEndpoints[12][2] = {
	{0, 1}, {1, 3}, {3, 2}, {2, 0},
	{4, 5}, {5, 7}, {7, 6}, {6, 4},
	{0, 4}, {1, 5}, {3, 7}, {2, 6}
};

struct EdgeCanonical {
	int dx, dy, dz, axis;
};

static const EdgeCanonical edgeCanonical[12] = {
	{0, 0, 0, 0}, {1, 0, 0, 1}, {0, 1, 0, 0}, {0, 0, 0, 1},
	{0, 0, 1, 0}, {1, 0, 1, 1}, {0, 1, 1, 0}, {0, 0, 1, 1},
	{0, 0, 0, 2}, {1, 0, 0, 2}, {1, 1, 0, 2}, {0, 1, 0, 2}
};

static inline int gridIndex(int x, int y, int z, int w, int h) {
	return x + y * w + z * w * h;
}

static inline uint64_t packEdgeKey(int x, int y, int z, int axis) {
	return (uint64_t)(uint16_t)x | ((uint64_t)(uint16_t)y << 16) | ((uint64_t)(uint16_t)z << 32) |
		   ((uint64_t)axis << 48);
}

static glm::vec3 computeGradient(const float *density, int x, int y, int z, int w, int h, int d) {
	const float dx = (x > 0 ? density[gridIndex(x - 1, y, z, w, h)] : density[gridIndex(x, y, z, w, h)]) -
					 (x < w - 1 ? density[gridIndex(x + 1, y, z, w, h)] : density[gridIndex(x, y, z, w, h)]);
	const float dy = (y > 0 ? density[gridIndex(x, y - 1, z, w, h)] : density[gridIndex(x, y, z, w, h)]) -
					 (y < h - 1 ? density[gridIndex(x, y + 1, z, w, h)] : density[gridIndex(x, y, z, w, h)]);
	const float dz = (z > 0 ? density[gridIndex(x, y, z - 1, w, h)] : density[gridIndex(x, y, z, w, h)]) -
					 (z < d - 1 ? density[gridIndex(x, y, z + 1, w, h)] : density[gridIndex(x, y, z, w, h)]);
	return glm::vec3(dx, dy, dz);
}

static uint8_t findNearestColor(const voxel::RawVolume *volume, const glm::vec3 &worldPos) {
	const voxel::Region &region = volume->region();
	const glm::ivec3 basePos = glm::ivec3(glm::floor(worldPos));

	if (region.containsPoint(basePos)) {
		const voxel::Voxel &v = volume->voxel(basePos);
		if (!voxel::isAir(v.getMaterial())) {
			return v.getColor();
		}
	}

	static constexpr int searchRadius = 2;
	static constexpr float MAX_SEARCH_DIST_SQ = 100.0f;
	float bestDistSq = MAX_SEARCH_DIST_SQ;
	uint8_t bestColor = 0;
	for (int dz = -searchRadius; dz <= searchRadius; ++dz) {
		for (int dy = -searchRadius; dy <= searchRadius; ++dy) {
			for (int dx = -searchRadius; dx <= searchRadius; ++dx) {
				const glm::ivec3 p = basePos + glm::ivec3(dx, dy, dz);
				if (!region.containsPoint(p)) {
					continue;
				}
				const voxel::Voxel &v = volume->voxel(p);
				if (voxel::isAir(v.getMaterial())) {
					continue;
				}
				const glm::vec3 diff = worldPos - glm::vec3(p);
				const float distSq = glm::dot(diff, diff);
				if (distSq < bestDistSq) {
					bestDistSq = distSq;
					bestColor = v.getColor();
				}
			}
		}
	}
	return bestColor;
}

// Taubin mesh smoothing: alternating shrink (lambda) and inflate (mu) steps.
// Respects mesh topology - vertices only move toward mesh neighbors, never through walls.
static void taubinSmooth(voxel::Mesh &mesh, int iterations, float lambda, float mu) {
	const size_t vertexCount = mesh.getNoOfVertices();
	const size_t indexCount = mesh.getNoOfIndices();
	if (vertexCount == 0 || indexCount == 0) {
		return;
	}

	// Build adjacency list from triangle indices.
	// Each vertex typically has ~6 neighbors, so flat arrays with linear-scan dedup
	// are faster than hash sets for iteration and have less overhead.
	Log::debug("Smooth mesh: building adjacency for %zu vertices...", vertexCount);
	core::DynamicArray<core::DynamicArray<voxel::IndexType>> adjacency;
	adjacency.resize(vertexCount);

	auto addNeighbor = [&adjacency](voxel::IndexType vertex, voxel::IndexType neighbor) {
		core::DynamicArray<voxel::IndexType> &neighbors = adjacency[vertex];
		for (const voxel::IndexType &existing : neighbors) {
			if (existing == neighbor) {
				return;
			}
		}
		neighbors.push_back(neighbor);
	};

	const voxel::IndexType *indices = mesh.getRawIndexData();
	for (size_t i = 0; i < indexCount; i += 3) {
		const voxel::IndexType i0 = indices[i + 0];
		const voxel::IndexType i1 = indices[i + 1];
		const voxel::IndexType i2 = indices[i + 2];
		addNeighbor(i0, i1);
		addNeighbor(i0, i2);
		addNeighbor(i1, i0);
		addNeighbor(i1, i2);
		addNeighbor(i2, i0);
		addNeighbor(i2, i1);
	}

	// Extract positions for smoothing (working copy)
	core::Buffer<glm::vec3> positions(vertexCount);
	core::Buffer<glm::vec3> smoothed(vertexCount);
	const voxel::VoxelVertex *vertices = mesh.getRawVertexData();
	for (size_t i = 0; i < vertexCount; ++i) {
		positions[i] = vertices[i].position;
	}

	auto smoothStep = [&](float factor) {
		for (size_t i = 0; i < vertexCount; ++i) {
			const core::DynamicArray<voxel::IndexType> &neighbors = adjacency[i];
			if (neighbors.empty()) {
				smoothed[i] = positions[i];
				continue;
			}
			glm::vec3 avg(0.0f);
			for (const voxel::IndexType &neighbor : neighbors) {
				avg += positions[neighbor];
			}
			avg /= (float)neighbors.size();
			smoothed[i] = positions[i] + factor * (avg - positions[i]);
		}
		// Swap
		for (size_t i = 0; i < vertexCount; ++i) {
			positions[i] = smoothed[i];
		}
	};

	const char *modeName = (mu == 0.0f) ? "Laplacian" : "Taubin";
	Log::debug("Smooth mesh: applying %d %s iterations (lambda=%.3f, mu=%.3f)", iterations, modeName, lambda, mu);
	for (int iter = 0; iter < iterations; ++iter) {
		smoothStep(lambda); // shrink
		if (mu != 0.0f) {
			smoothStep(mu); // inflate (mu < 0 prevents shrinkage)
		}
	}

	// Write smoothed positions back to mesh vertices
	voxel::VertexArray &vertArray = mesh.getVertexVector();
	for (size_t i = 0; i < vertexCount; ++i) {
		vertArray[i].position = positions[i];
	}
}

void extractSmoothMesh(const voxel::RawVolume *volume, int blurIterations,
					   SmoothFilter filter, float sharpness, voxel::ChunkMesh *result) {
	core_trace_scoped(ExtractSmoothMesh);

	const voxel::Region &region = volume->region();
	const glm::ivec3 &lower = region.getLowerCorner();

	// Grid with 1 voxel padding for MC boundary
	static constexpr int pad = 1;
	const int gridWidth = region.getWidthInVoxels() + 2 * pad;
	const int gridHeight = region.getHeightInVoxels() + 2 * pad;
	const int gridDepth = region.getDepthInVoxels() + 2 * pad;
	const size_t totalCells = (size_t)gridWidth * (size_t)gridHeight * (size_t)gridDepth;

	Log::debug("Smooth mesh: grid %dx%dx%d (%zu cells)", gridWidth, gridHeight, gridDepth, totalCells);

	// Fill binary density: solid = 1.0, air = 0.0
	core::Buffer<float> density(totalCells);
	for (size_t i = 0; i < totalCells; ++i) {
		density[i] = 0.0f;
	}

	const int volW = region.getWidthInVoxels();
	const int volH = region.getHeightInVoxels();
	const int volD = region.getDepthInVoxels();
	for (int z = 0; z < volD; ++z) {
		for (int y = 0; y < volH; ++y) {
			for (int x = 0; x < volW; ++x) {
				const glm::ivec3 pos = lower + glm::ivec3(x, y, z);
				const voxel::Voxel &v = volume->voxel(pos);
				if (!voxel::isAir(v.getMaterial())) {
					density[gridIndex(x + pad, y + pad, z + pad, gridWidth, gridHeight)] = 1.0f;
				}
			}
		}
	}

	// Marching cubes on the binary density field (threshold 0.5)
	static constexpr float threshold = 0.5f;
	voxel::Mesh &mesh = result->mesh[0];

	const int cellsX = gridWidth - 1;
	const int cellsY = gridHeight - 1;
	const int cellsZ = gridDepth - 1;

	Log::debug("Smooth mesh: running marching cubes on %dx%dx%d cells", cellsX, cellsY, cellsZ);

	std::unordered_map<uint64_t, voxel::IndexType> edgeVertexMap;
	edgeVertexMap.reserve(totalCells / 4);

	const glm::vec3 worldOffset = glm::vec3(lower) - glm::vec3((float)pad);

	for (int cz = 0; cz < cellsZ; ++cz) {
		for (int cy = 0; cy < cellsY; ++cy) {
			for (int cx = 0; cx < cellsX; ++cx) {
				float cornerDensity[8];
				glm::ivec3 cornerPos[8];
				for (int i = 0; i < 8; ++i) {
					cornerPos[i] = glm::ivec3(cx, cy, cz) + cornerOffsets[i];
					cornerDensity[i] = density[gridIndex(cornerPos[i].x, cornerPos[i].y, cornerPos[i].z, gridWidth, gridHeight)];
				}

				uint8_t cellIndex = 0;
				for (int i = 0; i < 8; ++i) {
					if (cornerDensity[i] < threshold) {
						cellIndex |= (uint8_t)(1 << i);
					}
				}

				const uint16_t edge = voxel::edgeTable[cellIndex];
				if (edge == 0) {
					continue;
				}

				static constexpr float EPSILON = 0.0001f;
				voxel::IndexType edgeVertIdx[12];

				for (int edgeIdx = 0; edgeIdx < 12; ++edgeIdx) {
					if (!(edge & (1 << edgeIdx))) {
						continue;
					}

					const EdgeCanonical &edgeInfo = edgeCanonical[edgeIdx];
					const uint64_t key = packEdgeKey(cx + edgeInfo.dx, cy + edgeInfo.dy, cz + edgeInfo.dz, edgeInfo.axis);

					auto it = edgeVertexMap.find(key);
					if (it != edgeVertexMap.end()) {
						edgeVertIdx[edgeIdx] = it->second;
						continue;
					}

					const int vertIdx0 = edgeEndpoints[edgeIdx][0];
					const int vertIdx1 = edgeEndpoints[edgeIdx][1];
					const float density0 = cornerDensity[vertIdx0];
					const float density1 = cornerDensity[vertIdx1];

					float interpolation = 0.5f;
					if (glm::abs(density1 - density0) > EPSILON) {
						interpolation = (threshold - density0) / (density1 - density0);
					}
					interpolation = glm::clamp(interpolation, 0.0f, 1.0f);

					const glm::vec3 p0(cornerPos[vertIdx0]);
					const glm::vec3 p1(cornerPos[vertIdx1]);
					const glm::vec3 gridPos = p0 + interpolation * (p1 - p0);

					const glm::vec3 normal0 = computeGradient(density.data(), cornerPos[vertIdx0].x, cornerPos[vertIdx0].y,
														 cornerPos[vertIdx0].z, gridWidth, gridHeight, gridDepth);
					const glm::vec3 normal1 = computeGradient(density.data(), cornerPos[vertIdx1].x, cornerPos[vertIdx1].y,
														 cornerPos[vertIdx1].z, gridWidth, gridHeight, gridDepth);
					glm::vec3 normal = normal0 + interpolation * (normal1 - normal0);
					const float len2 = glm::dot(normal, normal);
					if (len2 > EPSILON) {
						normal *= 1.0f / glm::sqrt(len2);
					} else {
						normal = glm::vec3(0.0f, 1.0f, 0.0f);
					}

					const glm::vec3 worldPos = gridPos + worldOffset;

					voxel::VoxelVertex vertex;
					vertex.position = worldPos;
					vertex.colorIndex = findNearestColor(volume, worldPos);
					vertex.normalIndex = NO_NORMAL;
					vertex.padding2 = 0;
					vertex.info = 0;

					const voxel::IndexType idx = mesh.addVertex(vertex);
					mesh.setNormal(idx, normal);
					edgeVertexMap[key] = idx;
					edgeVertIdx[edgeIdx] = idx;
				}

				for (int i = 0; voxel::triTable[cellIndex][i] != -1; i += 3) {
					const voxel::IndexType i0 = edgeVertIdx[voxel::triTable[cellIndex][i + 0]];
					const voxel::IndexType i1 = edgeVertIdx[voxel::triTable[cellIndex][i + 1]];
					const voxel::IndexType i2 = edgeVertIdx[voxel::triTable[cellIndex][i + 2]];
					mesh.addTriangle(i0, i1, i2);
				}
			}
		}
	}

	Log::debug("Smooth mesh: MC produced %zu vertices, %zu indices", mesh.getNoOfVertices(), mesh.getNoOfIndices());

	// Apply mesh smoothing: topology-aware, preserves thin walls
	if (blurIterations > 0 && mesh.getNoOfVertices() > 0) {
		static constexpr float TAUBIN_MU_OFFSET = 0.01f;
		const float lambda = glm::clamp(sharpness, 0.01f, 0.99f);
		if (filter == SmoothFilter::Taubin) {
			// Taubin: volume-preserving but mild. Good for removing noise without shrinking.
			const float mu = -lambda - TAUBIN_MU_OFFSET;
			taubinSmooth(mesh, blurIterations, lambda, mu);
		} else {
			// Laplacian: aggressive smoothing. Converts staircases into slopes.
			// Shrinks the mesh slightly - use lower lambda or fewer iterations to control.
			taubinSmooth(mesh, blurIterations, lambda, 0.0f);
		}

		// Recalculate normals after smoothing
		mesh.calculateNormals();
	}

	result->setOffset(lower);
	Log::info("Smooth mesh export: %zu vertices, %zu indices", mesh.getNoOfVertices(), mesh.getNoOfIndices());
}

} // namespace voxelutil
