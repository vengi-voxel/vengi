/**
 * @file
 */

#include "SurfaceExtractor.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "voxel/RawVolume.h"
#include "voxel/private/CubicSurfaceExtractor.h"
#include "voxel/private/MarchingCubesSurfaceExtractor.h"

namespace voxel {

SurfaceExtractionContext buildCubicContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
										   const glm::ivec3 &translate, bool mergeQuads, bool reuseVertices,
										   bool ambientOcclusion) {
	return SurfaceExtractionContext(volume, getPalette(), region, mesh, translate, SurfaceExtractionType::Cubic,
									mergeQuads, reuseVertices, ambientOcclusion);
}

SurfaceExtractionContext buildMarchingCubesContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
												   const palette::Palette &palette) {
	return SurfaceExtractionContext(volume, palette, region, mesh, glm::ivec3(0), SurfaceExtractionType::MarchingCubes,
									false, false, false);
}

void extractSurface(SurfaceExtractionContext &ctx) {
	if (ctx.type == SurfaceExtractionType::MarchingCubes) {
		voxel::Region extractRegion = ctx.region;
		extractRegion.shrink(-1);
		voxel::extractMarchingCubesMesh(ctx.volume, ctx.palette, extractRegion, &ctx.mesh);
	} else {
		voxel::Region extractRegion = ctx.region;
		extractRegion.shiftUpperCorner(1, 1, 1);
		voxel::extractCubicMesh(ctx.volume, extractRegion, &ctx.mesh, ctx.translate, ctx.mergeQuads, ctx.reuseVertices,
								ctx.ambientOcclusion);
	}
}

voxel::SurfaceExtractionContext createContext(voxel::SurfaceExtractionType type, const voxel::RawVolume *volume,
											  const voxel::Region &region, const palette::Palette &palette,
											  voxel::ChunkMesh &mesh, const glm::ivec3 &translate, bool mergeQuads,
											  bool reuseVertices, bool ambientOcclusion) {
	if (type == voxel::SurfaceExtractionType::MarchingCubes) {
		return voxel::buildMarchingCubesContext(volume, region, mesh, palette);
	}
	return voxel::buildCubicContext(volume, region, mesh, translate, mergeQuads, reuseVertices, ambientOcclusion);
}

} // namespace voxel
