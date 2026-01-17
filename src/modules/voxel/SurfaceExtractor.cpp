/**
 * @file
 */

#include "SurfaceExtractor.h"
#include "core/Log.h"
#include "palette/Palette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "voxel/RawVolume.h"
#include "voxel/private/BinaryGreedyMesher.h"
#include "voxel/private/CubicSurfaceExtractor.h"
#include "voxel/private/MarchingCubesSurfaceExtractor.h"

namespace voxel {

SurfaceExtractionContext buildCubicContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
										   const glm::ivec3 &translate, bool mergeQuads, bool reuseVertices,
										   bool ambientOcclusion, bool optimize) {
	static palette::Palette unused;
	return SurfaceExtractionContext(volume, unused, region, mesh, translate, SurfaceExtractionType::Cubic,
									mergeQuads, reuseVertices, ambientOcclusion, optimize);
}

SurfaceExtractionContext buildMarchingCubesContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
												   const palette::Palette &palette, bool optimize) {
	return SurfaceExtractionContext(volume, palette, region, mesh, glm::ivec3(0), SurfaceExtractionType::MarchingCubes,
									false, false, false, optimize);
}

SurfaceExtractionContext buildBinaryContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
										   const glm::ivec3 &translate, bool ambientOcclusion, bool optimize) {
	static palette::Palette unused;
	return SurfaceExtractionContext(volume, unused, region, mesh, translate, SurfaceExtractionType::Binary,
									false, false, ambientOcclusion, optimize);
}

void extractSurface(voxel::SurfaceExtractionContext &ctx) {
	core_assert_msg(ctx.volume != nullptr, "Provided volume cannot be null");

	ctx.mesh.clear();
	if (ctx.type == voxel::SurfaceExtractionType::MarchingCubes) {
		voxel::extractMarchingCubesMesh(ctx.volume, ctx.palette, ctx.region, &ctx.mesh);
	} else if (ctx.type == voxel::SurfaceExtractionType::Binary) {
		if (voxel::exceedsBinaryMesherRegion(ctx.region)) {
			Log::warn("Region %s exceeds the maximum size for the binary mesher - falling back to cubic mesher",
					  ctx.region.toString().c_str());
			voxel::extractCubicMesh(ctx.volume, ctx.region, &ctx.mesh, ctx.translate, ctx.ambientOcclusion, ctx.mergeQuads,
								ctx.reuseVertices);
		} else {
			voxel::extractBinaryGreedyMesh(ctx.volume, ctx.region, &ctx.mesh, ctx.translate, ctx.ambientOcclusion);
		}
	} else {
		voxel::extractCubicMesh(ctx.volume, ctx.region, &ctx.mesh, ctx.translate, ctx.ambientOcclusion, ctx.mergeQuads,
								ctx.reuseVertices);
	}
	if (ctx.optimize) {
		ctx.mesh.optimize();
	}
	ctx.mesh.removeUnusedVertices();
	ctx.mesh.compressIndices();
}

voxel::SurfaceExtractionContext createContext(voxel::SurfaceExtractionType type, const voxel::RawVolume *volume,
											  const voxel::Region &region, const palette::Palette &palette,
											  voxel::ChunkMesh &mesh, const glm::ivec3 &translate, bool mergeQuads,
											  bool reuseVertices, bool ambientOcclusion, bool optimize) {
	if (type == voxel::SurfaceExtractionType::MarchingCubes) {
		return voxel::buildMarchingCubesContext(volume, region, mesh, palette, optimize);
	} else if (type == voxel::SurfaceExtractionType::Binary) {
		return voxel::buildBinaryContext(volume, region, mesh, translate, ambientOcclusion, optimize);
	}
	return voxel::buildCubicContext(volume, region, mesh, translate, mergeQuads, reuseVertices, ambientOcclusion,
									optimize);
}

} // namespace voxel
