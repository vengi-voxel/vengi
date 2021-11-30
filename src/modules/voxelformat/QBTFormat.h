/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/FileStream.h"

namespace voxel {

/**
 * @brief Qubicle Binary Tree (qbt) is the successor of the widespread voxel exchange format Qubicle Binary.
 *
 * @see QBFormat
 *
 * https://getqubicle.com/qubicle/documentation/docs/file/qbt/
 */
class QBTFormat : public VoxFileFormat {
private:
	bool skipNode(io::ReadStream& stream);
	bool loadMatrix(io::ReadStream& stream, VoxelVolumes& volumes);
	bool loadCompound(io::ReadStream& stream, VoxelVolumes& volumes);
	bool loadModel(io::ReadStream& stream, VoxelVolumes& volumes);
	bool loadNode(io::ReadStream& stream, VoxelVolumes& volumes);

	bool loadColorMap(io::ReadStream& stream);
	bool loadFromStream(io::ReadStream& stream, VoxelVolumes& volumes);
	bool saveMatrix(io::FileStream& stream, const VoxelVolume& volume, bool colorMap) const;
	bool saveColorMap(io::FileStream& stream) const;
	bool saveModel(io::FileStream& stream, const VoxelVolumes& volumes, bool colorMap) const;
public:
	bool loadGroups(const core::String &filename, io::ReadStream& stream, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
