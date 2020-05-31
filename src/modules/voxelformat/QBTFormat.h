/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"

namespace voxel {

/**
 * @brief Qubicle Binary Tree (qbt) is the successor of the widespread voxel exchange format Qubicle Binary.
 *
 * @see QBFormat
 *
 * http://minddesk.com/learn/article.php?id=47
 */
class QBTFormat : public VoxFileFormat {
private:
	bool skipNode(io::FileStream& stream);
	bool loadMatrix(io::FileStream& stream, VoxelVolumes& volumes);
	bool loadCompound(io::FileStream& stream, VoxelVolumes& volumes);
	bool loadModel(io::FileStream& stream, VoxelVolumes& volumes);
	bool loadNode(io::FileStream& stream, VoxelVolumes& volumes);

	bool loadColorMap(io::FileStream& stream);
	bool loadFromStream(io::FileStream& stream, VoxelVolumes& volumes);
	bool saveMatrix(io::FileStream& stream, const VoxelVolume& volume, bool colorMap) const;
	bool saveColorMap(io::FileStream& stream) const;
	bool saveModel(io::FileStream& stream, const VoxelVolumes& volumes, bool colorMap) const;
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
