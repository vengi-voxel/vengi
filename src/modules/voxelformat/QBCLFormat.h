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

	bool readMatrix(io::ReadStream &stream);
	bool readModel(io::ReadStream &stream);
	bool readCompound(io::ReadStream &stream);
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::ReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::ReadStream& stream, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
