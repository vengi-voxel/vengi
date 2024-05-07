/**
 * @file
 * @ingroup Formats
 */

#pragma once

#include "Format.h"
#include "io/File.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "video/Texture.h"

namespace palette {
class Palette;
}

namespace voxelformat {

const io::FormatDescription *voxelLoad();
const io::FormatDescription *voxelSave();
const io::FormatDescription &aceOfSpades();
const io::FormatDescription &tiberianSun();
const io::FormatDescription &qubicleBinary();
const io::FormatDescription &qubicleBinaryTree();
const io::FormatDescription &magicaVoxel();
const io::FormatDescription &vengi();
const io::FormatDescription &gltf();

/**
 * @brief Tries to load a palette from the given file. This can either be an image which is reduced to 256 colors or a
 * volume format with an embedded palette
 */
bool importPalette(const core::String &filename, palette::Palette &palette);
/**
 * @brief Tries to load the embedded palette from the given file. If the format doesn't have a palette embedded, this
 * returns @c 0
 */
size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
				   const LoadContext &ctx);
image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive, const LoadContext &ctx);
bool loadFormat(const io::FileDescription &fileDesc, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
				const LoadContext &ctx);

/**
 * @brief Save both to volume or to mesh - depends on the given file extension
 */
bool saveFormat(const io::FilePtr &filePtr, const io::FormatDescription *desc, scenegraph::SceneGraph &sceneGraph,
				const SaveContext &ctx, bool useVengiAsFallback = true);
bool saveFormat(scenegraph::SceneGraph &sceneGraph, const core::String &filename, const io::FormatDescription *desc,
				const io::ArchivePtr &archive, const SaveContext &ctx);

bool isMeshFormat(const core::String &filename, bool save);
bool isMeshFormat(const io::FormatDescription &desc);
bool isAnimationSupported(const io::FormatDescription &desc);
bool isModelFormat(const core::String &filename);

} // namespace voxelformat
