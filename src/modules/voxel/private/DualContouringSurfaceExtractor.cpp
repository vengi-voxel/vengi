/**
 * @file
 */

#include "core/Assert.h"
#include "core/Pair.h"
#include "palette/Palette.h"
#include "voxel/Mesh.h"
#include "voxel/QEF.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelVertex.h"
#include "voxel/ChunkMesh.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/ext/scalar_common.hpp>
#include <glm/gtx/norm.hpp>

// BUG We will get duplication of edges if the surface is along region boundaries

namespace voxel {
namespace {

struct PositionNormal {
	glm::vec3 position;
	glm::vec3 normal;
};

struct EdgeData {
	glm::vec3 normal{0.0f};
	/** fraction (0.0-1.0) along the edge in the positive direction that the intersection happens */
	float fraction = 0.0f;
	bool intersects = false;
};

struct CellData {
	EdgeData edges[3];
	IndexType vertexIndex;
};

using ThresholdType = float;

static const float DualContouringMaxDensity = 255.0f;

static inline float convertToDensity(const Voxel &voxel) {
	return isAir(voxel.getMaterial()) ? 0.0f : DualContouringMaxDensity;
}

static inline EdgeData calculateEdge(const float &vA, const float &vB, const glm::vec3 &gA, const glm::vec3 &gB,
									 const ThresholdType &threshold) {
	EdgeData edge;

	const float divisor = (vA - vB);
	if (divisor == 0.0f) {
		edge.fraction = 0.0f;
	} else {
		edge.fraction = (vA - threshold) / divisor;
	}

	if (glm::min(vA, vB) <= threshold && glm::max(vA, vB) > threshold) {
		edge.intersects = true;
	} else {
		edge.intersects = false;
		return edge;
	}

	edge.normal = (gA * edge.fraction + gB * (1.0f - edge.fraction));
	const float v = glm::length2(edge.normal);
	if (v != 0.0f) {
		edge.normal *= glm::inversesqrt(v);
	}

	return edge;
}

static inline PositionNormal computeVertex(EdgeData *edges[12]) {
	glm::vec3 massPoint{0.0f}; // The average of the intersection vertices

	glm::vec3 vertices[12];

	vertices[0] = {edges[0]->fraction, 0, 0};
	vertices[1] = {0, edges[1]->fraction, 0};
	vertices[2] = {0, 0, edges[2]->fraction};
	vertices[3] = {1, edges[3]->fraction, 0};
	vertices[4] = {1, 0, edges[4]->fraction};
	vertices[5] = {0, 1, edges[5]->fraction};
	vertices[6] = {edges[6]->fraction, 1, 0};
	vertices[7] = {edges[7]->fraction, 0, 1};
	vertices[8] = {0, edges[8]->fraction, 1};
	vertices[9] = {1, 1, edges[9]->fraction};
	vertices[10] = {1, edges[10]->fraction, 1};
	vertices[11] = {edges[11]->fraction, 1, 1};

	int numIntersections = 0;
	for (int i = 0; i < 12; ++i) {
		if (!edges[i]->intersects) {
			continue;
		}

		++numIntersections;
		massPoint += vertices[i];
	}

	massPoint /= numIntersections; // Make the average

	glm::vec3 cellVertexNormal{0, 0, 0};

	double matrix[12][3];
	double vector[12];
	int rows = 0;

	for (int i = 0; i < 12; ++i) {
		if (!edges[i]->intersects) {
			continue;
		}

		glm::vec3 normal = edges[i]->normal;
		matrix[rows][0] = normal.x;
		matrix[rows][1] = normal.y;
		matrix[rows][2] = normal.z;

		const glm::vec3 product = normal * (vertices[i] - massPoint);

		vector[rows] = product.x + product.y + product.z;

		cellVertexNormal += normal;

		++rows;
	}

	const glm::vec3 &vertexPosition = evaluateQEF(matrix, vector, rows) + massPoint;

	core_assert_msg(vertexPosition.x > -0.01 && vertexPosition.y > -0.01 && vertexPosition.z > -0.01 &&
						vertexPosition.x < 1.01 && vertexPosition.y < 1.01 && vertexPosition.z < 1.01,
					"Vertex is outside unit cell %f:%f:%f", vertexPosition.x, vertexPosition.y, vertexPosition.z);

	const float v = glm::length2(cellVertexNormal);
	if (v != 0.0f) {
		cellVertexNormal *= glm::inversesqrt(v);
	}

	return {vertexPosition, cellVertexNormal};
}

static inline uint32_t convert(uint32_t x, uint32_t y, uint32_t z, uint32_t width, uint32_t height) {
	return z * height * width + y * width + x;
}

} // namespace

void extractDualContouringMesh(const voxel::RawVolume *volData, const palette::Palette &palette, const Region &region,
							   ChunkMesh *result) {
	const float threshold = 127.5f; // controller.getThreshold();

	const int regionXDimension = region.getDimensionsInVoxels().x;
	const int regionYDimension = region.getDimensionsInVoxels().y;
	const int regionZDimension = region.getDimensionsInVoxels().z;

	const auto gradientRegionXDimension = regionXDimension + 2;
	const auto gradientRegionYDimension = regionYDimension + 2;
	const auto gradientRegionZDimension = regionZDimension + 2;

	const size_t gradientsSize =
		(size_t)gradientRegionXDimension * (size_t)gradientRegionYDimension * (size_t)gradientRegionZDimension;
	core::DynamicArray<core::Pair<float, glm::vec3>> gradients;
	gradients.reserve(gradientsSize);

	RawVolume::Sampler volSampler{volData};
	volSampler.setPosition(region.getLowerCorner() - 1);

	const auto lowerCornerX = region.getLowerCorner().x;
	const auto lowerCornerY = region.getLowerCorner().y;
	const auto lowerCornerZ = region.getLowerCorner().z;

	for (int32_t z = 0; z < gradientRegionZDimension; z++) {
		volSampler.setPosition(lowerCornerX - 1, lowerCornerY - 1,
							   lowerCornerZ + z - 1); // Reset x and y and increment z
		for (int32_t y = 0; y < gradientRegionYDimension; y++) {
			volSampler.setPosition(lowerCornerX - 1, lowerCornerY + y - 1,
								   lowerCornerZ + z - 1); // Reset x and increment y (z remains the same)
			for (int32_t x = 0; x < gradientRegionXDimension; x++) {
				volSampler.movePositiveX(); // Increment x

				const float voxel = convertToDensity(volSampler.voxel());
				const float voxel1px = convertToDensity(volSampler.peekVoxel1px0py0pz());
				const float voxel1py = convertToDensity(volSampler.peekVoxel0px1py0pz());
				const float voxel1pz = convertToDensity(volSampler.peekVoxel0px0py1pz());
				const float voxel1nx = convertToDensity(volSampler.peekVoxel1nx0py0pz());
				const float voxel1ny = convertToDensity(volSampler.peekVoxel0px1ny0pz());
				const float voxel1nz = convertToDensity(volSampler.peekVoxel0px0py1nz());

				gradients.emplace_back(voxel, glm::vec3(voxel1nx - voxel1px, voxel1ny - voxel1py, voxel1nz - voxel1pz));
			}
		}
	}

	const int32_t cellRegionXDimension = regionXDimension + 2;
	const int32_t cellRegionYDimension = regionYDimension + 2;
	const int32_t cellRegionZDimension = regionZDimension + 2;

	const size_t cellSize = (size_t)cellRegionXDimension * (size_t)cellRegionYDimension * (size_t)cellRegionZDimension;
	core::DynamicArray<CellData> cells;
	cells.reserve(cellSize);

	for (int32_t cellZ = 0; cellZ < cellRegionZDimension; cellZ++) {
		for (int32_t cellY = 0; cellY < cellRegionYDimension; cellY++) {
			for (int32_t cellX = 0; cellX < cellRegionXDimension; cellX++) {
				// For each cell, calculate the edge intersection points and normals
				const auto &g000 = gradients[convert(cellX, cellY, cellZ, cellRegionXDimension, cellRegionYDimension)];

				// For the last columns/rows, only calculate the interior edge
				if (cellX < cellRegionXDimension - 1 && cellY < cellRegionYDimension - 1 &&
					cellZ < cellRegionZDimension - 1) // This is the main bulk
				{
					const auto &g100 =
						gradients[convert(cellX + 1, cellY, cellZ, cellRegionXDimension, cellRegionYDimension)];
					const auto &g010 =
						gradients[convert(cellX, cellY + 1, cellZ, cellRegionXDimension, cellRegionYDimension)];
					const auto &g001 =
						gradients[convert(cellX, cellY, cellZ + 1, cellRegionXDimension, cellRegionYDimension)];
					cells.push_back({{calculateEdge(g000.first, g100.first, g000.second, g100.second, threshold),
									  calculateEdge(g000.first, g010.first, g000.second, g010.second, threshold),
									  calculateEdge(g000.first, g001.first, g000.second, g001.second, threshold)},
									 0});
				} else if (cellX == cellRegionXDimension - 1 || cellY == cellRegionYDimension - 1 ||
						   cellZ == cellRegionZDimension - 1) // This is the three far edges and the far corner
				{
					cells.push_back({});					  // Default and empty
				} else if (cellX == cellRegionXDimension - 1) // Far x side
				{
					const auto &g100 =
						gradients[convert(cellX + 1, cellY, cellZ, cellRegionXDimension, cellRegionYDimension)];
					cells.push_back({{calculateEdge(g000.first, g100.first, g000.second, g100.second, threshold),
									  EdgeData(), EdgeData()},
									 0});
				} else if (cellY == cellRegionYDimension - 1) // Far y side
				{
					const auto &g010 =
						gradients[convert(cellX + 1, cellY, cellZ, cellRegionXDimension, cellRegionYDimension)];
					cells.push_back(
						{{EdgeData(), calculateEdge(g000.first, g010.first, g000.second, g010.second, threshold),
						  EdgeData()},
						 0});
				} else if (cellZ == cellRegionZDimension - 1) // Far z side
				{
					const auto &g001 =
						gradients[convert(cellX + 1, cellY, cellZ, cellRegionXDimension, cellRegionYDimension)];
					cells.push_back({{EdgeData(), EdgeData(),
									  calculateEdge(g000.first, g001.first, g000.second, g001.second, threshold)},
									 0});
				}
			}
		}
	}

	EdgeData *edges[12]; // Create this now but it will be overwritten for each cell

	for (int32_t cellZ = 0; cellZ < cellRegionZDimension; cellZ++) {
		for (int32_t cellY = 0; cellY < cellRegionYDimension; cellY++) {
			for (int32_t cellX = 0; cellX < cellRegionXDimension; cellX++) {
				if (cellZ >= 1 && cellY >= 1 && cellX >= 1) {
					// After the first rows and columns are done, start calculating vertex positions
					const int32_t cellXVertex = cellX - 1;
					const int32_t cellYVertex = cellY - 1;
					const int32_t cellZVertex = cellZ - 1;

					auto &cell = cells[convert(cellXVertex, cellYVertex, cellZVertex, cellRegionXDimension,
											   cellRegionYDimension)];

					edges[0] = &cell.edges[0];
					edges[1] = &cell.edges[1];
					edges[2] = &cell.edges[2];

					edges[3] = &cells[convert(cellXVertex + 1, cellYVertex, cellZVertex, cellRegionXDimension,
											  cellRegionYDimension)]
									.edges[1];
					edges[4] = &cells[convert(cellXVertex + 1, cellYVertex, cellZVertex, cellRegionXDimension,
											  cellRegionYDimension)]
									.edges[2];

					edges[5] = &cells[convert(cellXVertex, cellYVertex + 1, cellZVertex, cellRegionXDimension,
											  cellRegionYDimension)]
									.edges[2];
					edges[6] = &cells[convert(cellXVertex, cellYVertex + 1, cellZVertex, cellRegionXDimension,
											  cellRegionYDimension)]
									.edges[0];

					edges[7] = &cells[convert(cellXVertex, cellYVertex, cellZVertex + 1, cellRegionXDimension,
											  cellRegionYDimension)]
									.edges[0];
					edges[8] = &cells[convert(cellXVertex, cellYVertex, cellZVertex + 1, cellRegionXDimension,
											  cellRegionYDimension)]
									.edges[1];

					edges[9] = &cells[convert(cellXVertex + 1, cellYVertex + 1, cellZVertex, cellRegionXDimension,
											  cellRegionYDimension)]
									.edges[2];

					edges[10] = &cells[convert(cellXVertex + 1, cellYVertex, cellZVertex + 1, cellRegionXDimension,
											   cellRegionYDimension)]
									 .edges[1];

					edges[11] = &cells[convert(cellXVertex, cellYVertex + 1, cellZVertex + 1, cellRegionXDimension,
											   cellRegionYDimension)]
									 .edges[0];

					if (edges[0]->intersects || edges[1]->intersects || edges[2]->intersects || edges[3]->intersects ||
						edges[4]->intersects || edges[5]->intersects || edges[6]->intersects || edges[7]->intersects ||
						edges[8]->intersects || edges[9]->intersects || edges[10]->intersects ||
						edges[11]->intersects) //'if' Maybe not needed?
					{
						const PositionNormal &vertex = computeVertex(edges);

						VoxelVertex v;
						v.info = 0;

						// TODO: use the sampler
						const int x = region.getLowerCorner().x + cellXVertex;
						const int y = region.getLowerCorner().y + cellYVertex;
						const int z = region.getLowerCorner().z + cellZVertex;
						v.colorIndex = volData->voxel(x, y, z).getColor();
						v.position.x = vertex.position.x + (float)cellXVertex;
						v.position.y = vertex.position.y + (float)cellYVertex;
						v.position.z = vertex.position.z + (float)cellZVertex;

						cell.vertexIndex = result->mesh[0].addVertex(v);
						result->mesh[0].setNormal(cell.vertexIndex, vertex.normal);

						// TODO: transparency mesh
						if (cellZVertex >= 1 && cellYVertex >= 1 && cellXVertex >= 1) {
							// Once the second rows and colums are done, start connecting up edges
							if (cell.edges[0].intersects) {
								const auto &v1 = cells[convert(cellXVertex, cellYVertex - 1, cellZVertex,
															   cellRegionXDimension, cellRegionYDimension)];
								const auto &v2 = cells[convert(cellXVertex, cellYVertex, cellZVertex - 1,
															   cellRegionXDimension, cellRegionYDimension)];
								const auto &v3 = cells[convert(cellXVertex, cellYVertex - 1, cellZVertex - 1,
															   cellRegionXDimension, cellRegionYDimension)];
								result->mesh[0].addTriangle(cell.vertexIndex, v1.vertexIndex, v2.vertexIndex);
								result->mesh[0].addTriangle(v3.vertexIndex, v2.vertexIndex, v1.vertexIndex);
							}

							if (cell.edges[1].intersects) {
								const auto &v1 = cells[convert(cellXVertex - 1, cellYVertex, cellZVertex,
															   cellRegionXDimension, cellRegionYDimension)];
								const auto &v2 = cells[convert(cellXVertex, cellYVertex, cellZVertex - 1,
															   cellRegionXDimension, cellRegionYDimension)];
								const auto &v3 = cells[convert(cellXVertex - 1, cellYVertex, cellZVertex - 1,
															   cellRegionXDimension, cellRegionYDimension)];
								result->mesh[0].addTriangle(cell.vertexIndex, v1.vertexIndex, v2.vertexIndex);
								result->mesh[0].addTriangle(v3.vertexIndex, v2.vertexIndex, v1.vertexIndex);
							}

							if (cell.edges[2].intersects) {
								const auto &v1 = cells[convert(cellXVertex - 1, cellYVertex, cellZVertex,
															   cellRegionXDimension, cellRegionYDimension)];
								const auto &v2 = cells[convert(cellXVertex, cellYVertex - 1, cellZVertex,
															   cellRegionXDimension, cellRegionYDimension)];
								const auto &v3 = cells[convert(cellXVertex - 1, cellYVertex - 1, cellZVertex,
															   cellRegionXDimension, cellRegionYDimension)];
								result->mesh[0].addTriangle(cell.vertexIndex, v1.vertexIndex, v2.vertexIndex);
								result->mesh[0].addTriangle(v3.vertexIndex, v2.vertexIndex, v1.vertexIndex);
							}
						}
					}
				}
			}
		}
	}
}

} // namespace voxel
