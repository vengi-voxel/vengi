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

	bool readMatrix(io::FileStream &stream);
	bool readModel(io::FileStream &stream);
	bool readCompound(io::FileStream &stream);
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
