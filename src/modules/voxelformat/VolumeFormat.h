/**
 * @file
 */

#pragma once

#include "SceneGraph.h"
#include "Format.h"
#include "core/collection/Array.h"
#include "io/File.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "video/Texture.h"

namespace voxel {
class Palette;
}

namespace voxelformat {

extern const io::FormatDescription* voxelLoad();
extern const io::FormatDescription* voxelSave();
extern io::FormatDescription tiberianSun();
extern io::FormatDescription qubicleBinary();
extern io::FormatDescription vengi();


/**
 * @brief Tries to load a palette from the given file. This can either be an image which is reduced to 256 colors or a
 * volume format with an embedded palette
 */
extern bool importPalette(const core::String &filename, voxel::Palette &palette);
/**
 * @brief Tries to load the embedded palette from the given file. If the format doesn't have a palette embedded, this returns @c 0
 */
extern size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette, const LoadContext &ctx);
extern image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream, const LoadContext &ctx);
extern bool loadFormat(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph, const LoadContext &ctx);

/**
 * @brief Save both to volume or to mesh - depends on the given file extension
 */
extern bool saveFormat(const io::FilePtr &filePtr, const io::FormatDescription *desc, SceneGraph &sceneGraph, const SaveContext &ctx);
extern bool saveFormat(SceneGraph &sceneGraph, const core::String &filename, const io::FormatDescription *desc, io::SeekableWriteStream &stream, const SaveContext &ctx);

extern bool isMeshFormat(const core::String &filename);
extern bool isMeshFormat(const io::FormatDescription &desc);
extern bool isModelFormat(const core::String &filename);

} // namespace voxelformat
