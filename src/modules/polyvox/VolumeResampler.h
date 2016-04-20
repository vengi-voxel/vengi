#pragma once

#include "Region.h"
#include "Interpolation.h"
#include "core/Common.h"
#include <cmath>

namespace PolyVox {

/**
 * This class can be used to copy data from one volume to another, possibly while
 * resizing it. It has not been heavily used an may or may not work as expected.
 */
template<typename SrcVolumeType, typename DstVolumeType>
class VolumeResampler {
public:
	VolumeResampler(SrcVolumeType* pVolSrc, const Region& regSrc, DstVolumeType* pVolDst, const Region& regDst);

	void execute();

private:
	void resampleSameSize();
	void resampleArbitrary();

	//Source data
	SrcVolumeType* m_pVolSrc;
	Region m_regSrc;

	//Destination data
	DstVolumeType* m_pVolDst;
	Region m_regDst;
};

template<typename SrcVolumeType, typename DstVolumeType>
VolumeResampler<SrcVolumeType, DstVolumeType>::VolumeResampler(SrcVolumeType* pVolSrc, const Region &regSrc, DstVolumeType* pVolDst, const Region& regDst) :
		m_pVolSrc(pVolSrc), m_regSrc(regSrc), m_pVolDst(pVolDst), m_regDst(regDst) {
}

template<typename SrcVolumeType, typename DstVolumeType>
void VolumeResampler<SrcVolumeType, DstVolumeType>::execute() {
	const int32_t uSrcWidth = m_regSrc.getUpperX() - m_regSrc.getLowerX() + 1;
	const int32_t uSrcHeight = m_regSrc.getUpperY() - m_regSrc.getLowerY() + 1;
	const int32_t uSrcDepth = m_regSrc.getUpperZ() - m_regSrc.getLowerZ() + 1;

	const int32_t uDstWidth = m_regDst.getUpperX() - m_regDst.getLowerX() + 1;
	const int32_t uDstHeight = m_regDst.getUpperY() - m_regDst.getLowerY() + 1;
	const int32_t uDstDepth = m_regDst.getUpperZ() - m_regDst.getLowerZ() + 1;

	if (uSrcWidth == uDstWidth && uSrcHeight == uDstHeight && uSrcDepth == uDstDepth) {
		resampleSameSize();
	} else {
		resampleArbitrary();
	}
}

template<typename SrcVolumeType, typename DstVolumeType>
void VolumeResampler<SrcVolumeType, DstVolumeType>::resampleSameSize() {
	for (int32_t sz = m_regSrc.getLowerZ(), dz = m_regDst.getLowerZ(); dz <= m_regDst.getUpperZ(); sz++, dz++) {
		for (int32_t sy = m_regSrc.getLowerY(), dy = m_regDst.getLowerY(); dy <= m_regDst.getUpperY(); sy++, dy++) {
			for (int32_t sx = m_regSrc.getLowerX(), dx = m_regDst.getLowerX(); dx <= m_regDst.getUpperX(); sx++, dx++) {
				const typename SrcVolumeType::VoxelType& tSrcVoxel = m_pVolSrc->getVoxel(sx, sy, sz);
				const typename DstVolumeType::VoxelType& tDstVoxel = static_cast<typename DstVolumeType::VoxelType>(tSrcVoxel);
				m_pVolDst->setVoxel(dx, dy, dz, tDstVoxel);
			}
		}
	}
}

template<typename SrcVolumeType, typename DstVolumeType>
void VolumeResampler<SrcVolumeType, DstVolumeType>::resampleArbitrary() {
	const float srcWidth = m_regSrc.getWidthInCells();
	const float srcHeight = m_regSrc.getHeightInCells();
	const float srcDepth = m_regSrc.getDepthInCells();

	const float dstWidth = m_regDst.getWidthInCells();
	const float dstHeight = m_regDst.getHeightInCells();
	const float dstDepth = m_regDst.getDepthInCells();

	const float fScaleX = srcWidth / dstWidth;
	const float fScaleY = srcHeight / dstHeight;
	const float fScaleZ = srcDepth / dstDepth;

	typename SrcVolumeType::Sampler sampler(m_pVolSrc);

	for (int32_t dz = m_regDst.getLowerZ(); dz <= m_regDst.getUpperZ(); dz++) {
		for (int32_t dy = m_regDst.getLowerY(); dy <= m_regDst.getUpperY(); dy++) {
			for (int32_t dx = m_regDst.getLowerX(); dx <= m_regDst.getUpperX(); dx++) {
				float sx = (dx - m_regDst.getLowerX()) * fScaleX;
				float sy = (dy - m_regDst.getLowerY()) * fScaleY;
				float sz = (dz - m_regDst.getLowerZ()) * fScaleZ;

				sx += m_regSrc.getLowerX();
				sy += m_regSrc.getLowerY();
				sz += m_regSrc.getLowerZ();

				sampler.setPosition(sx, sy, sz);
				const typename SrcVolumeType::VoxelType& voxel000 = sampler.peekVoxel0px0py0pz();
				const typename SrcVolumeType::VoxelType& voxel001 = sampler.peekVoxel0px0py1pz();
				const typename SrcVolumeType::VoxelType& voxel010 = sampler.peekVoxel0px1py0pz();
				const typename SrcVolumeType::VoxelType& voxel011 = sampler.peekVoxel0px1py1pz();
				const typename SrcVolumeType::VoxelType& voxel100 = sampler.peekVoxel1px0py0pz();
				const typename SrcVolumeType::VoxelType& voxel101 = sampler.peekVoxel1px0py1pz();
				const typename SrcVolumeType::VoxelType& voxel110 = sampler.peekVoxel1px1py0pz();
				const typename SrcVolumeType::VoxelType& voxel111 = sampler.peekVoxel1px1py1pz();

				float dummy;
				sx = glm::modf(sx, dummy);
				sy = glm::modf(sy, dummy);
				sz = glm::modf(sz, dummy);

				typename SrcVolumeType::VoxelType tInterpolatedValue = trilerp<typename SrcVolumeType::VoxelType>(voxel000, voxel100, voxel010, voxel110, voxel001, voxel101, voxel011, voxel111, sx, sy, sz);
				typename DstVolumeType::VoxelType result = static_cast<typename DstVolumeType::VoxelType>(tInterpolatedValue);
				m_pVolDst->setVoxel(dx, dy, dz, result);
			}
		}
	}
}

}
