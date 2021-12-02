/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/FileStream.h"

namespace voxel {

/**
 * @brief Qubicle project file (qbcl) format.
 *
 * Not yet implemented
 */
class QBCLFormat : public VoxFileFormat {
private:
	uint32_t _version;

	bool readMatrix(io::SeekableReadStream &stream);
	bool readModel(io::SeekableReadStream &stream);
	bool readCompound(io::SeekableReadStream &stream);
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
