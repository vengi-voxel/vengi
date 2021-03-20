/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/FileStream.h"

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
 * https://getqubicle.com/qubicle/documentation/docs/file/qef/
 */
class QEFFormat : public VoxFileFormat {
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
