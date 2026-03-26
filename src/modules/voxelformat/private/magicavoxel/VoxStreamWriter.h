/**
 * @file
 * @brief Streaming VOX file writer that writes model data directly from RawVolume
 * without allocating dense voxel arrays. This avoids the memory overhead of the OGT
 * library's ogt_vox_write_scene which requires all models in dense format simultaneously.
 */

#pragma once

#include "io/Stream.h"

namespace scenegraph {
class SceneGraph;
}

namespace palette {
class Palette;
}

namespace voxelformat {

struct MVSceneContext;

/**
 * @brief Write a complete VOX file using streaming model writes.
 *
 * Instead of converting all models to dense arrays and passing them to OGT's
 * ogt_vox_write_scene, this writes SIZE+XYZI chunks directly from RawVolume
 * references, one model at a time. The scene graph (nTRN/nGRP/nSHP), palette
 * (RGBA), materials (MATL), layers (LAYR), cameras (rCAM), and color names
 * (NOTE) are written inline.
 *
 * @param s The seekable write stream (must support seek for chunk size patching)
 * @param ctx The scene context populated by VoxFormat::saveNode
 * @param sceneGraph The scene graph (used to read palette)
 * @return true on success
 */
bool saveGroupsStreamed(io::SeekableWriteStream *s, const MVSceneContext &ctx, const scenegraph::SceneGraph &sceneGraph);

} // namespace voxelformat
