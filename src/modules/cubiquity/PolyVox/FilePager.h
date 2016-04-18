#pragma once

#include "PagedVolume.h"
#include "Region.h"

#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <vector>

namespace PolyVox {
/**
 * An implementation of Pager which stores voxels to files on disk. Each chunk is written
 * to a seperate file and you can specify the name of a folder where these will be stored.
 *
 * Note that no compression is performed (mostly to avoid dependancies) so for large
 * volumes you may want to consider this class as an example and create a custom version
 * with compression.
 */
template<typename VoxelType>
class FilePager: public PagedVolume<VoxelType>::Pager {
public:
	/// Constructor
	FilePager(const std::string& strFolderName = ".") :
			PagedVolume<VoxelType>::Pager(), m_strFolderName(strFolderName) {
		// Add the trailing slash, assuming the user dind't already do it.
		if (m_strFolderName.back() != '/' && m_strFolderName.back() != '\\') {
			m_strFolderName.append("/");
		}

		// Build a unique postfix to avoid filename conflicts between multiple pagers/runs.
		// Not a very robust solution but this class is meant as an example for testing really.
		std::stringstream ss;
		ss << time(0) << "--"; // Avoid multiple runs using the same filenames.
		ss << this; // Avoid multiple FilePagers using the same filenames.
		m_strPostfix = ss.str();
	}

	/// Destructor
	virtual ~FilePager() {
		for (std::vector<std::string>::iterator iter = m_vecCreatedFiles.begin(); iter < m_vecCreatedFiles.end(); iter++) {
			if (std::remove(iter->c_str()) != 0) {
				::Log::warn("Failed to delete '%s' when destroying FilePager", iter->c_str());
			}
		}

		m_vecCreatedFiles.clear();
	}

	virtual void pageIn(const Region& region, typename PagedVolume<VoxelType>::Chunk* pChunk) {
		core_assert_msg(pChunk, "Attempting to page in NULL chunk");
		core_assert_msg(pChunk->getData(), "Chunk must have valid data");

		std::stringstream ssFilename;
		ssFilename << m_strFolderName << "/" << region.getLowerX() << "_" << region.getLowerY() << "_" << region.getLowerZ() << "_" << region.getUpperX() << "_"
				<< region.getUpperY() << "_" << region.getUpperZ() << "--" << m_strPostfix;

		std::string filename = ssFilename.str();

		// FIXME - This should be replaced by C++ style IO, but currently this causes problems with
		// the gameplay-cubiquity integration. See: https://github.com/blackberry/GamePlay/issues/919

		FILE* pFile = fopen(filename.c_str(), "rb");
		if (pFile) {
			POLYVOX_LOG_TRACE("Paging in data for ", region);

			fread(pChunk->getData(), sizeof(uint8_t), pChunk->getDataSizeInBytes(), pFile);

			if (ferror(pFile)) {
				core_assert_msg(false, "Error reading in chunk data, even though a file exists.");
			}

			fclose(pFile);
		} else {
			POLYVOX_LOG_TRACE("No data found for ", region, " during paging in.");

			// Just fill with zeros. This feels hacky... perhaps we should just throw
			// an exception and let the calling code handle it and fill with zeros.
			uint32_t noOfVoxels = region.getWidthInVoxels() * region.getHeightInVoxels() * region.getDepthInVoxels();
			std::fill(pChunk->getData(), pChunk->getData() + noOfVoxels, VoxelType());
		}
	}

	virtual void pageOut(const Region& region, typename PagedVolume<VoxelType>::Chunk* pChunk) {
		core_assert_msg(pChunk, "Attempting to page out NULL chunk");
		core_assert_msg(pChunk->getData(), "Chunk must have valid data");

		POLYVOX_LOG_TRACE("Paging out data for ", region);

		std::stringstream ssFilename;
		ssFilename << m_strFolderName << "/" << region.getLowerX() << "_" << region.getLowerY() << "_" << region.getLowerZ() << "_" << region.getUpperX() << "_"
				<< region.getUpperY() << "_" << region.getUpperZ() << "--" << m_strPostfix;

		std::string filename = ssFilename.str();

		// FIXME - This should be replaced by C++ style IO, but currently this causes problems with
		// the gameplay-cubiquity integration. See: https://github.com/blackberry/GamePlay/issues/919

		FILE* pFile = fopen(filename.c_str(), "wb");
		if (!pFile) {
			core_assert_msg(false, "Unable to open file to write out chunk data.");
		}

		//The file has been created, so add it to the list to delete on shutdown.
		m_vecCreatedFiles.push_back(filename);

		fwrite(pChunk->getData(), sizeof(uint8_t), pChunk->getDataSizeInBytes(), pFile);

		if (ferror(pFile)) {
			core_assert_msg(false, "Error writing out chunk data.");
		}

		fclose(pFile);
	}

protected:
	std::string m_strFolderName;
	std::string m_strPostfix;

	std::vector<std::string> m_vecCreatedFiles;
};
}
