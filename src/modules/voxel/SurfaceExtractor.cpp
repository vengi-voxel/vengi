/**
 * @file
 */

#include "SurfaceExtractor.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "voxel/private/CubicSurfaceExtractor.h"
#include "voxel/private/MarchingCubesSurfaceExtractor.h"

namespace voxel {

SurfaceExtractionContext buildCubicContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
										   const glm::ivec3 &translate, bool mergeQuads, bool reuseVertices,
										   bool ambientOcclusion) {
	return SurfaceExtractionContext(volume, getPalette(), region, mesh, translate, false, mergeQuads, reuseVertices,
									ambientOcclusion);
}

SurfaceExtractionContext buildMarchingCubesContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
												   const Palette &palette) {
	return SurfaceExtractionContext(volume, palette, region, mesh, glm::ivec3(0), true, false, false, false);
}

void extractSurface(SurfaceExtractionContext &ctx) {
	if (ctx.marchingCubes) {
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

} // namespace voxel
