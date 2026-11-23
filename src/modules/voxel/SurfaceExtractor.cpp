/**
 * @file
 */

#include "SurfaceExtractor.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/Var.h"
#include "palette/Palette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "voxel/RawVolume.h"
#include "voxel/private/BinaryGreedyMesher.h"
#include "voxel/private/CubicSurfaceExtractor.h"
#include "voxel/private/DualContouringSurfaceExtractor.h"
#include "voxel/private/MarchingCubesSurfaceExtractor.h"
#include "voxel/private/TextureSurfaceExtractor.h"
#include "math/Vec3i.h"

namespace voxel {

SurfaceExtractionContext buildCubicContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
										   const math::Vec3i &translate, bool mergeQuads, bool reuseVertices,
										   bool ambientOcclusion, bool optimize) {
	static palette::Palette unused;
	return SurfaceExtractionContext(volume, unused, region, mesh, translate, SurfaceExtractionType::Cubic,
									mergeQuads, reuseVertices, ambientOcclusion, optimize);
}

SurfaceExtractionContext buildMarchingCubesContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
												   const palette::Palette &palette, bool optimize) {
	return SurfaceExtractionContext(volume, palette, region, mesh, math::Vec3i(0), SurfaceExtractionType::MarchingCubes,
									false, false, false, optimize);
}

SurfaceExtractionContext buildGreedyTextureContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
												   const palette::Palette &palette, bool optimize) {
	const bool textureDedupe = core::getVar(cfg::VoxelTextureDedupe)->boolVal();
	return SurfaceExtractionContext(volume, palette, region, mesh, math::Vec3i(0), SurfaceExtractionType::GreedyTexture,
									false, false, false, optimize, textureDedupe);
}

SurfaceExtractionContext buildBinaryContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
										   const math::Vec3i &translate, bool ambientOcclusion, bool optimize) {
	static palette::Palette unused;
	return SurfaceExtractionContext(volume, unused, region, mesh, translate, SurfaceExtractionType::Binary,
									false, false, ambientOcclusion, optimize);
}

SurfaceExtractionContext buildDualContouringContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
													const math::Vec3i &translate, bool optimize) {
	static palette::Palette unused;
	return SurfaceExtractionContext(volume, unused, region, mesh, translate, SurfaceExtractionType::DualContouring,
									false, false, false, optimize);
}

void extractSurface(voxel::SurfaceExtractionContext &ctx) {
	core_assert_msg(ctx.volume != nullptr, "Provided volume cannot be null");
	const math::Vec3i &mins = ctx.region.getLowerCorner();
	const math::Vec3i &maxs = ctx.region.getUpperCorner();
	Log::debug("Extracting surface with mode %i in region [%d, %d, %d] - [%d, %d, %d]", (int)ctx.type, mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);

	ctx.mesh.clear();
	if (ctx.type == voxel::SurfaceExtractionType::MarchingCubes) {
		voxel::extractMarchingCubesMesh(ctx.volume, ctx.palette, ctx.region, &ctx.mesh);
	} else if (ctx.type == SurfaceExtractionType::DualContouring) {
		extractDualContouringMesh(ctx);
	} else if (ctx.type == voxel::SurfaceExtractionType::GreedyTexture) {
		voxel::extractTextureMesh(ctx);
	} else if (ctx.type == voxel::SurfaceExtractionType::Binary) {
		if (voxel::exceedsBinaryMesherRegion(ctx.region)) {
			const auto &regions = getBinaryMesherRegions(ctx.region);
			for (const voxel::Region &r : regions) {
				Log::debug("extract region %s", r.toString().c_str());
				voxel::extractBinaryGreedyMesh(ctx.volume, r.getLowerCorner(), &ctx.mesh, ctx.translate + r.getLowerCorner(), ctx.ambientOcclusion, ctx.preserveVoxelNormals);
			}
			ctx.mesh.setOffset(ctx.region.getLowerCorner());
		} else {
			voxel::extractBinaryGreedyMesh(ctx.volume, ctx.region.getLowerCorner(), &ctx.mesh, ctx.translate, ctx.ambientOcclusion, ctx.preserveVoxelNormals);
		}
	} else {
		voxel::extractCubicMesh(ctx.volume, ctx.region, &ctx.mesh, ctx.translate, ctx.ambientOcclusion, ctx.mergeQuads,
								ctx.reuseVertices, ctx.preserveVoxelNormals);
	}
	if (ctx.optimize) {
		ctx.mesh.optimize();
	}
	// Cubic mergeQuads can leave unreferenced vertices; other extractors emit only used verts.
	if (ctx.type == voxel::SurfaceExtractionType::Cubic) {
		ctx.mesh.removeUnusedVertices();
	}
	ctx.mesh.compressIndices();
}

voxel::SurfaceExtractionContext createContext(voxel::SurfaceExtractionType type, const voxel::RawVolume *volume,
											  const voxel::Region &region, const palette::Palette &palette,
											  voxel::ChunkMesh &mesh, const math::Vec3i &translate, bool mergeQuads,
											  bool reuseVertices, bool ambientOcclusion, bool optimize) {
	if (type == voxel::SurfaceExtractionType::MarchingCubes) {
		return voxel::buildMarchingCubesContext(volume, region, mesh, palette, optimize);
	} else if (type == voxel::SurfaceExtractionType::Binary) {
		return voxel::buildBinaryContext(volume, region, mesh, translate, ambientOcclusion, optimize);
	} else if (type == voxel::SurfaceExtractionType::GreedyTexture) {
		return voxel::buildGreedyTextureContext(volume, region, mesh, palette, optimize);
	} else if (type == SurfaceExtractionType::DualContouring) {
		return buildDualContouringContext(volume, region, mesh, translate, optimize);
	}
	return voxel::buildCubicContext(volume, region, mesh, translate, mergeQuads, reuseVertices, ambientOcclusion,
									optimize);
}

} // namespace voxel
