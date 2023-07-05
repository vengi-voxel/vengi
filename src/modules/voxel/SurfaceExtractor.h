/**
 * @file
 */

#pragma once

namespace voxel {
class RawVolume;
class Region;
class Palette;
struct ChunkMesh;

struct SurfaceExtractionContext {
	SurfaceExtractionContext(const RawVolume *volume, const Palette &palette, const Region &region, ChunkMesh &mesh,
							 const glm::ivec3 &translate, bool marchingCubes, bool mergeQuads, bool reuseVertices,
							 bool ambientOcclusion)
		: volume(volume), palette(palette), region(region), mesh(mesh), translate(translate),
		  marchingCubes(marchingCubes), mergeQuads(mergeQuads), reuseVertices(reuseVertices),
		  ambientOcclusion(ambientOcclusion) {
	}
	const RawVolume *volume;
	const Palette &palette;
	const Region &region;
	ChunkMesh &mesh;
	const glm::ivec3 translate;
	const bool marchingCubes;
	const bool mergeQuads;
	const bool reuseVertices;
	const bool ambientOcclusion;
};

SurfaceExtractionContext buildCubicContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
										   const glm::ivec3 &translate = glm::ivec3(0), bool mergeQuads = true,
										   bool reuseVertices = true, bool ambientOcclusion = true);
SurfaceExtractionContext buildMarchingCubesContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
												   const Palette &palette);

void extractSurface(SurfaceExtractionContext &ctx);

} // namespace voxel
