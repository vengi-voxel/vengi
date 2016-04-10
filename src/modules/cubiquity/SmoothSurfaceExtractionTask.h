#pragma once

#include "CubiquityForwardDeclarations.h"
#include "OctreeNode.h"
#include "Task.h"
#include "core/Common.h"

namespace Cubiquity {
class SmoothSurfaceExtractionTask: public Task {
public:
	SmoothSurfaceExtractionTask(OctreeNode<MaterialSet>* octreeNode, ::PolyVox::PagedVolume<MaterialSet>* polyVoxVolume);
	virtual ~SmoothSurfaceExtractionTask();

	void process(void) override;

	void generateSmoothMesh(const Region& region, uint32_t lodLevel, TerrainMesh* resultMesh);

public:
	OctreeNode<MaterialSet>* _octreeNode;
	::PolyVox::PagedVolume<MaterialSet>* _polyVoxVolume;
	TerrainMesh* _polyVoxMesh;
	Timestamp _processingStartedTimestamp;

	// Whether the task owns the mesh, or whether it has been passed to
	// the OctreeNode. Should probably switch this to use a smart pointer.
	bool _ownMesh;
};

void recalculateMaterials(TerrainMesh* mesh, const Vector3F& meshOffset, ::PolyVox::PagedVolume<MaterialSet>* volume);
MaterialSet getInterpolatedValue(::PolyVox::PagedVolume<MaterialSet>* volume, const Vector3F& position);

template<typename SrcPolyVoxVolumeType, typename DstPolyVoxVolumeType>
void resampleVolume(uint32_t factor, SrcPolyVoxVolumeType* srcVolume, const Region& srcRegion, DstPolyVoxVolumeType* dstVolume, const Region& dstRegion) {
	core_assert_msg((uint32_t)srcRegion.getWidthInCells() == dstRegion.getWidthInCells() * factor, "Destination volume must be half the size of source volume");
	core_assert_msg((uint32_t)srcRegion.getHeightInCells() == dstRegion.getHeightInCells() * factor, "Destination volume must be half the size of source volume");
	core_assert_msg((uint32_t)srcRegion.getDepthInCells() == dstRegion.getDepthInCells() * factor, "Destination volume must be half the size of source volume");

	for (int32_t dz = dstRegion.getLowerCorner().getZ(); dz <= dstRegion.getUpperCorner().getZ(); dz++) {
		for (int32_t dy = dstRegion.getLowerCorner().getY(); dy <= dstRegion.getUpperCorner().getY(); dy++) {
			for (int32_t dx = dstRegion.getLowerCorner().getX(); dx <= dstRegion.getUpperCorner().getX(); dx++) {
				int32_t sx = (dx - dstRegion.getLowerCorner().getX()) * factor + srcRegion.getLowerCorner().getX();
				int32_t sy = (dy - dstRegion.getLowerCorner().getY()) * factor + srcRegion.getLowerCorner().getY();
				int32_t sz = (dz - dstRegion.getLowerCorner().getZ()) * factor + srcRegion.getLowerCorner().getZ();

				const MaterialSet& srcVoxel = srcVolume->getVoxel(sx, sy, sz);
				dstVolume->setVoxel(dx, dy, dz, srcVoxel);
			}
		}
	}
}

}
