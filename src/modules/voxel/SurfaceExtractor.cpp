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
										   bool ambientOcclusion, bool optimize) {
	return SurfaceExtractionContext(volume, getPalette(), region, mesh, translate, SurfaceExtractionType::Cubic,
									mergeQuads, reuseVertices, ambientOcclusion, optimize);
}

SurfaceExtractionContext buildMarchingCubesContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
												   const palette::Palette &palette, bool optimize) {
	return SurfaceExtractionContext(volume, palette, region, mesh, glm::ivec3(0), SurfaceExtractionType::MarchingCubes,
									false, false, false, optimize);
}

void extractSurface(SurfaceExtractionContext &ctx) {
	if (ctx.type == SurfaceExtractionType::MarchingCubes) {
		voxel::extractMarchingCubesMesh(ctx.volume, ctx.palette, ctx.region, &ctx.mesh, ctx.optimize);
	} else {
		voxel::extractCubicMesh(ctx.volume, ctx.region, &ctx.mesh, ctx.translate, ctx.mergeQuads, ctx.reuseVertices,
								ctx.ambientOcclusion, ctx.optimize);
	}
}

voxel::SurfaceExtractionContext createContext(voxel::SurfaceExtractionType type, const voxel::RawVolume *volume,
											  const voxel::Region &region, const palette::Palette &palette,
											  voxel::ChunkMesh &mesh, const glm::ivec3 &translate, bool mergeQuads,
											  bool reuseVertices, bool ambientOcclusion, bool optimize) {
	if (type == voxel::SurfaceExtractionType::MarchingCubes) {
		return voxel::buildMarchingCubesContext(volume, region, mesh, palette, optimize);
	}
	return voxel::buildCubicContext(volume, region, mesh, translate, mergeQuads, reuseVertices, ambientOcclusion, optimize);
}

} // namespace voxel
