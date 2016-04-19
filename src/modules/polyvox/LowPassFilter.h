#pragma once

#include "IteratorController.h"
#include "RawVolume.h" // Currently used by exectureSAT() method - should be replaced by PagedVolume or a template parameter?

#include "Region.h"

namespace PolyVox {
/// This class is able to copy volume data from a source volume to a destination volume while performing low-pass filtering (blurring).
template<typename SrcVolumeType, typename DstVolumeType, typename AccumulationType>
class LowPassFilter {
public:
	LowPassFilter(SrcVolumeType* pVolSrc, Region regSrc, DstVolumeType* pVolDst, Region regDst, uint32_t uKernelSize);

	/// Execute a standard approach to filtering which performs a number of neighbourhood look-ups per voxel.
	void execute();
	/// Execute a version with 'Summed Area Tables'. This should be faster for large kernel sizes but this hasn't really been confirmed yet.
	void executeSAT();

private:
	//Source data
	SrcVolumeType* m_pVolSrc;
	Region m_regSrc;

	//Destination data
	DstVolumeType* m_pVolDst;
	Region m_regDst;

	//Kernel size
	uint32_t m_uKernelSize;
};

/**
 * @param pVolSrc
 * @param regSrc
 * @param[out] pVolDst
 * @param regDst
 * @param uKernelSize
 */
template<typename SrcVolumeType, typename DstVolumeType, typename AccumulationType>
LowPassFilter<SrcVolumeType, DstVolumeType, AccumulationType>::LowPassFilter(SrcVolumeType* pVolSrc, Region regSrc, DstVolumeType* pVolDst, Region regDst, uint32_t uKernelSize) :
		m_pVolSrc(pVolSrc), m_regSrc(regSrc), m_pVolDst(pVolDst), m_regDst(regDst), m_uKernelSize(uKernelSize) {
	//Kernel size must be at least three
	core_assert_msg(m_uKernelSize >= 3, "Kernel size must be at least three");

	//Kernel size must be odd
	core_assert_msg(m_uKernelSize % 2 == 1, "Kernel size must be odd");
}

template<typename SrcVolumeType, typename DstVolumeType, typename AccumulationType>
void LowPassFilter<SrcVolumeType, DstVolumeType, AccumulationType>::execute() {
	const int32_t iSrcMinX = m_regSrc.getLowerX();
	const int32_t iSrcMinY = m_regSrc.getLowerY();
	const int32_t iSrcMinZ = m_regSrc.getLowerZ();

	const int32_t iSrcMaxX = m_regSrc.getUpperX();
	const int32_t iSrcMaxY = m_regSrc.getUpperY();
	const int32_t iSrcMaxZ = m_regSrc.getUpperZ();

	const int32_t iDstMinX = m_regDst.getLowerX();
	const int32_t iDstMinY = m_regDst.getLowerY();
	const int32_t iDstMinZ = m_regDst.getLowerZ();

	//const int32_t iDstMaxX = m_regDst.getUpperX();
	//const int32_t iDstMaxY = m_regDst.getUpperY();
	//const int32_t iDstMaxZ = m_regDst.getUpperZ();

	typename SrcVolumeType::Sampler srcSampler(m_pVolSrc);

	for (int32_t iSrcZ = iSrcMinZ, iDstZ = iDstMinZ; iSrcZ <= iSrcMaxZ; iSrcZ++, iDstZ++) {
		for (int32_t iSrcY = iSrcMinY, iDstY = iDstMinY; iSrcY <= iSrcMaxY; iSrcY++, iDstY++) {
			for (int32_t iSrcX = iSrcMinX, iDstX = iDstMinX; iSrcX <= iSrcMaxX; iSrcX++, iDstX++) {
				AccumulationType tSrcVoxel(0);
				srcSampler.setPosition(iSrcX, iSrcY, iSrcZ);

				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1nx1ny1nz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1nx1ny0pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1nx1ny1pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1nx0py1nz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1nx0py0pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1nx0py1pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1nx1py1nz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1nx1py0pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1nx1py1pz());

				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel0px1ny1nz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel0px1ny0pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel0px1ny1pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel0px0py1nz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel0px0py0pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel0px0py1pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel0px1py1nz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel0px1py0pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel0px1py1pz());

				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1px1ny1nz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1px1ny0pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1px1ny1pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1px0py1nz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1px0py0pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1px0py1pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1px1py1nz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1px1py0pz());
				tSrcVoxel += static_cast<AccumulationType>(srcSampler.peekVoxel1px1py1pz());

				tSrcVoxel /= 27;

				//tSrcVoxel.setDensity(uDensity);
				m_pVolDst->setVoxel(iSrcX, iSrcY, iSrcZ, static_cast<typename DstVolumeType::VoxelType>(tSrcVoxel));
			}
		}
	}
}

template<typename SrcVolumeType, typename DstVolumeType, typename AccumulationType>
void LowPassFilter<SrcVolumeType, DstVolumeType, AccumulationType>::executeSAT() {
	const uint32_t border = (m_uKernelSize - 1) / 2;

	const glm::ivec3 satLowerCorner = m_regSrc.getLowerCorner() - glm::ivec3(border, border, border);
	const glm::ivec3 satUpperCorner = m_regSrc.getUpperCorner() + glm::ivec3(border, border, border);

	//Use floats for the SAT volume to ensure it works with negative
	//densities and with both integral and floating point input volumes.
	RawVolume<AccumulationType> satVolume(Region(satLowerCorner, satUpperCorner));

	//Clear to zeros (necessary?)
	//FIXME - use Volume::fill() method. Implemented in base class as below
	//but with optimised implementations in subclasses?
	for (int32_t z = satLowerCorner.z; z <= satUpperCorner.z; z++) {
		for (int32_t y = satLowerCorner.y; y <= satUpperCorner.y; y++) {
			for (int32_t x = satLowerCorner.x; x <= satUpperCorner.x; x++) {
				satVolume.setVoxel(x, y, z, 0);
			}
		}
	}

	typename RawVolume<AccumulationType>::Sampler satVolumeIter(&satVolume);

	IteratorController<typename RawVolume<AccumulationType>::Sampler> satIterCont;
	satIterCont.m_regValid = Region(satLowerCorner, satUpperCorner);
	satIterCont.m_Iter = &satVolumeIter;
	satIterCont.reset();

	typename SrcVolumeType::Sampler srcVolumeIter(m_pVolSrc);

	IteratorController<typename SrcVolumeType::Sampler> srcIterCont;
	srcIterCont.m_regValid = Region(satLowerCorner, satUpperCorner);
	srcIterCont.m_Iter = &srcVolumeIter;
	srcIterCont.reset();

	do {
		AccumulationType previousSum = static_cast<AccumulationType>(satVolumeIter.peekVoxel1nx0py0pz());
		AccumulationType currentVal = static_cast<AccumulationType>(srcVolumeIter.getVoxel());

		satVolumeIter.setVoxel(previousSum + currentVal);

		srcIterCont.moveForward();

	} while (satIterCont.moveForward());

	//Build SAT in three passes
	/*for(int32_t z = satLowerCorner.z; z <= satUpperCorner.z; z++)
	 {
	 for(int32_t y = satLowerCorner.y; y <= satUpperCorner.y; y++)
	 {
	 for(int32_t x = satLowerCorner.x; x <= satUpperCorner.x; x++)
	 {
	 AccumulationType previousSum = static_cast<AccumulationType>(satVolume.getVoxel(x-1,y,z));
	 AccumulationType currentVal = static_cast<AccumulationType>(m_pVolSrc->getVoxel(x,y,z));

	 satVolume.setVoxel(x,y,z,previousSum + currentVal);
	 }
	 }
	 }*/

	for (int32_t z = satLowerCorner.z; z <= satUpperCorner.z; z++) {
		for (int32_t y = satLowerCorner.y; y <= satUpperCorner.y; y++) {
			for (int32_t x = satLowerCorner.x; x <= satUpperCorner.x; x++) {
				AccumulationType previousSum = static_cast<AccumulationType>(satVolume.getVoxel(x, y - 1, z));
				AccumulationType currentSum = static_cast<AccumulationType>(satVolume.getVoxel(x, y, z));

				satVolume.setVoxel(x, y, z, previousSum + currentSum);
			}
		}
	}

	for (int32_t z = satLowerCorner.z; z <= satUpperCorner.z; z++) {
		for (int32_t y = satLowerCorner.y; y <= satUpperCorner.y; y++) {
			for (int32_t x = satLowerCorner.x; x <= satUpperCorner.x; x++) {
				AccumulationType previousSum = static_cast<AccumulationType>(satVolume.getVoxel(x, y, z - 1));
				AccumulationType currentSum = static_cast<AccumulationType>(satVolume.getVoxel(x, y, z));

				satVolume.setVoxel(x, y, z, previousSum + currentSum);
			}
		}
	}

	//Now compute the average
	const glm::ivec3& v3dDstLowerCorner = m_regDst.getLowerCorner();
	const glm::ivec3& v3dDstUpperCorner = m_regDst.getUpperCorner();
	const glm::ivec3& v3dSrcLowerCorner = m_regSrc.getLowerCorner();

	for (int32_t iDstZ = v3dDstLowerCorner.z, iSrcZ = v3dSrcLowerCorner.z; iDstZ <= v3dDstUpperCorner.z; iDstZ++, iSrcZ++) {
		for (int32_t iDstY = v3dDstLowerCorner.y, iSrcY = v3dSrcLowerCorner.y; iDstY <= v3dDstUpperCorner.y; iDstY++, iSrcY++) {
			for (int32_t iDstX = v3dDstLowerCorner.x, iSrcX = v3dSrcLowerCorner.x; iDstX <= v3dDstUpperCorner.x; iDstX++, iSrcX++) {
				const int32_t satLowerX = iSrcX - border - 1;
				const int32_t satLowerY = iSrcY - border - 1;
				const int32_t satLowerZ = iSrcZ - border - 1;

				const int32_t satUpperX = iSrcX + border;
				const int32_t satUpperY = iSrcY + border;
				const int32_t satUpperZ = iSrcZ + border;

				AccumulationType a = satVolume.getVoxel(satLowerX, satLowerY, satLowerZ);
				AccumulationType b = satVolume.getVoxel(satUpperX, satLowerY, satLowerZ);
				AccumulationType c = satVolume.getVoxel(satLowerX, satUpperY, satLowerZ);
				AccumulationType d = satVolume.getVoxel(satUpperX, satUpperY, satLowerZ);
				AccumulationType e = satVolume.getVoxel(satLowerX, satLowerY, satUpperZ);
				AccumulationType f = satVolume.getVoxel(satUpperX, satLowerY, satUpperZ);
				AccumulationType g = satVolume.getVoxel(satLowerX, satUpperY, satUpperZ);
				AccumulationType h = satVolume.getVoxel(satUpperX, satUpperY, satUpperZ);

				AccumulationType sum = h + c - d - g - f - a + b + e;
				uint32_t sideLength = border * 2 + 1;
				AccumulationType average = sum / (sideLength * sideLength * sideLength);

				m_pVolDst->setVoxel(iDstX, iDstY, iDstZ, static_cast<typename DstVolumeType::VoxelType>(average));
			}
		}
	}
}

}
