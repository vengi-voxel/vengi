/**
 * @file
 * @ingroup Formats
 */

#pragma once

#include "Format.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"

namespace palette {
class Palette;
}

namespace voxelformat {

const io::FormatDescription *voxelLoad();
const io::FormatDescription *voxelSave();

const io::FormatDescription &tiberianSun();
const io::FormatDescription &qubicleBinary();
const io::FormatDescription &magicaVoxel();
const io::FormatDescription &qubicleBinaryTree();
const io::FormatDescription &vengi();
const io::FormatDescription &aseprite();
const io::FormatDescription &roomsThing();
const io::FormatDescription &qubicleProject();
const io::FormatDescription &qubicleExchange();
const io::FormatDescription &ufoaiBsp();
const io::FormatDescription &quake1Bsp();
const io::FormatDescription &quakeMdl();
const io::FormatDescription &quakeMd2();
const io::FormatDescription &blockbench();
const io::FormatDescription &sandboxVXM();
const io::FormatDescription &sandboxVXB();
const io::FormatDescription &sandboxVXR();
const io::FormatDescription &sandboxTilemap();
const io::FormatDescription &sandboxCollection();
const io::FormatDescription &binvox();
const io::FormatDescription &goxel();
const io::FormatDescription &cubeWorld();
const io::FormatDescription &minetest();
const io::FormatDescription &minecraftRegion();
const io::FormatDescription &minecraftLevelDat();
const io::FormatDescription &minecraftSchematic();
const io::FormatDescription &fbx();
const io::FormatDescription &autodesk3ds();
const io::FormatDescription &sproxelCSV();
const io::FormatDescription &magicaVoxelXRAW();
const io::FormatDescription &gltf();
const io::FormatDescription &particubes();
const io::FormatDescription &cubzhB64();
const io::FormatDescription &cubzh();
const io::FormatDescription &aceOfSpadesKV6();
const io::FormatDescription &voxelMax();
const io::FormatDescription &starMade();
const io::FormatDescription &starMadeTemplate();
const io::FormatDescription &animaToon();
const io::FormatDescription &voxelBuilder();
const io::FormatDescription &wavefrontObj();
const io::FormatDescription &standardTriangleLanguage();
const io::FormatDescription &polygonFileFormat();
const io::FormatDescription &buildKVX();
const io::FormatDescription &voxel3D();
const io::FormatDescription &chronoVox();
const io::FormatDescription &nicksVoxelModel();
const io::FormatDescription &slab6Vox();

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
bool saveFormat(scenegraph::SceneGraph &sceneGraph, const core::String &filename, const io::FormatDescription *desc,
				const io::ArchivePtr &archive, const SaveContext &ctx);

bool isMeshFormat(const core::String &filename, bool save);
bool isMeshFormat(const io::FormatDescription &desc);
bool isAnimationSupported(const io::FormatDescription &desc);
bool isModelFormat(const core::String &filename);
bool isPaletteEmbedded(const io::FormatDescription &desc);
bool isRGBFormat(const io::FormatDescription &desc);

} // namespace voxelformat
