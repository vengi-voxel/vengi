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
	bool loadMatrix(io::FileStream& stream);
	bool loadCompound(io::FileStream& stream);
	bool loadModel(io::FileStream& stream);
	bool loadNode(io::FileStream& stream);
	bool loadFromStream(io::FileStream& stream);
public:
	RawVolume* load(const io::FilePtr& file) override;
	bool save(const RawVolume* volume, const io::FilePtr& file) override;
};

}
