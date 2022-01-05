/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {

/**
 * @brief Qubicle project file (qbcl) format.
 *
 * Not yet implemented
 */
class QBCLFormat : public Format {
private:
	bool readMatrix(io::SeekableReadStream &stream);
	bool readModel(io::SeekableReadStream &stream);
	bool readCompound(io::SeekableReadStream &stream);
public:
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) override;
	bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& volumes) override;
	bool saveGroups(const SceneGraph& volumes, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
