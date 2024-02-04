/**
 * @file
 */

#pragma once

#include "math/Math.h"

namespace palette {
class Palette;
}

namespace voxel {
class RawVolume;
class Region;
struct ChunkMesh;

enum class SurfaceExtractionType { Cubic, MarchingCubes, Max };

struct SurfaceExtractionContext {
	SurfaceExtractionContext(const RawVolume *_volume, const palette::Palette &_palette, const Region &_region,
							 ChunkMesh &_mesh, const glm::ivec3 &_translate, SurfaceExtractionType _type,
							 bool _mergeQuads, bool _reuseVertices, bool _ambientOcclusion)
		: volume(_volume), palette(_palette), region(_region), mesh(_mesh), translate(_translate), type(_type),
		  mergeQuads(_mergeQuads), reuseVertices(_reuseVertices), ambientOcclusion(_ambientOcclusion) {
	}
	const RawVolume *volume;
	const palette::Palette &palette;
	const Region &region;
	ChunkMesh &mesh;
	const glm::ivec3 translate;
	const SurfaceExtractionType type;
	const bool mergeQuads;
	const bool reuseVertices;
	const bool ambientOcclusion;
};

SurfaceExtractionContext buildCubicContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
										   const glm::ivec3 &translate = glm::ivec3(0), bool mergeQuads = true,
										   bool reuseVertices = true, bool ambientOcclusion = true);
SurfaceExtractionContext buildMarchingCubesContext(const RawVolume *volume, const Region &region, ChunkMesh &mesh,
												   const palette::Palette &palette);

void extractSurface(SurfaceExtractionContext &ctx);

} // namespace voxel
