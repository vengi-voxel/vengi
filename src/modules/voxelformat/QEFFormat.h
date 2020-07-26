/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"

namespace voxel {

/**
 * @brief Qubicle Exchange (QEF)
 *
 * QEF is a rather old ASCII exchange format originally developed for the unreleased Qubicle Plugin for Maya. It is
 * recommended to use the newer and more flexible Qubicle Binary exchange format instead.
 *
 * @see QBTFormat
 * @see QBFormat
 *
 * http://minddesk.com/learn/article.php?id=23
 */
class QEFFormat : public VoxFileFormat {
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
