#pragma once

#include "Color.h"
#include "MaterialSet.h"

namespace Cubiquity {

// We use traits to decide (for example) which vertex type should correspond to a given voxel type,
// or which surface extractor should be used for a given voxel type. Maybe it is useful to consider
// putting some of this (the VoxelType to VertexType maybe?) into PolyVox.
template<typename Type>
class VoxelTraits;

template<>
class VoxelTraits<Color> {
public:
	typedef ColoredCubesVertex VertexType;
	typedef ColoredCubicSurfaceExtractionTask SurfaceExtractionTaskType;
	static const bool IsColor = true;
	static const bool IsMaterialSet = false;
};

template<>
class VoxelTraits<MaterialSet> {
public:
	typedef TerrainVertex VertexType;
	typedef SmoothSurfaceExtractionTask SurfaceExtractionTaskType;
	static const bool IsColor = false;
	static const bool IsMaterialSet = true;
};

}
