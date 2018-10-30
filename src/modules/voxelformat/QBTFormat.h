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
 * http://minddesk.com/learn/article.php?id=47
 */
class QBTFormat : public VoxFileFormat {
private:
	bool skipNode(io::FileStream& stream);
	bool loadMatrix(io::FileStream& stream, std::vector<RawVolume*>& volumes);
	bool loadCompound(io::FileStream& stream, std::vector<RawVolume*>& volumes);
	bool loadModel(io::FileStream& stream, std::vector<RawVolume*>& volumes);
	bool loadNode(io::FileStream& stream, std::vector<RawVolume*>& volumes);

	bool loadColorMap(io::FileStream& stream);
	bool loadFromStream(io::FileStream& stream, std::vector<RawVolume*>& volumes);
public:
	std::vector<RawVolume*> loadGroups(const io::FilePtr& file) override;
	bool save(const RawVolume* volume, const io::FilePtr& file) override;
};

}
