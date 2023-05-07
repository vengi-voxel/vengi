/**
 * @file
 * @ingroup Formats
 */

#pragma once

#include "Format.h"
#include "core/collection/Array.h"
#include "io/File.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "video/Texture.h"

namespace voxel {
class Palette;
}

namespace voxelformat {

const io::FormatDescription *voxelLoad();
const io::FormatDescription *voxelSave();
io::FormatDescription aceOfSpades();
io::FormatDescription tiberianSun();
io::FormatDescription qubicleBinary();
io::FormatDescription qubicleBinaryTree();
io::FormatDescription magicaVoxel();
io::FormatDescription vengi();

/**
 * @brief Tries to load a palette from the given file. This can either be an image which is reduced to 256 colors or a
 * volume format with an embedded palette
 */
bool importPalette(const core::String &filename, voxel::Palette &palette);
/**
 * @brief Tries to load the embedded palette from the given file. If the format doesn't have a palette embedded, this
 * returns @c 0
 */
size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
				   const LoadContext &ctx);
image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream, const LoadContext &ctx);
bool loadFormat(const io::FileDescription &fileDesc, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
				const LoadContext &ctx);

/**
 * @brief Save both to volume or to mesh - depends on the given file extension
 */
bool saveFormat(const io::FilePtr &filePtr, const io::FormatDescription *desc, scenegraph::SceneGraph &sceneGraph,
				const SaveContext &ctx);
bool saveFormat(scenegraph::SceneGraph &sceneGraph, const core::String &filename, const io::FormatDescription *desc,
				io::SeekableWriteStream &stream, const SaveContext &ctx);

bool isMeshFormat(const core::String &filename, bool save);
bool isMeshFormat(const io::FormatDescription &desc);
bool isModelFormat(const core::String &filename);

} // namespace voxelformat
