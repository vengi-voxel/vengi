/**
 * @file
 */

#pragma once

#include "VXLShared.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"

namespace voxelformat {

/**
 * @brief Hierarchical Voxel Animation Format
 *
 * @li https://modenc.renegadeprojects.com/HVA
 *
 * HVA files (Hierarchical Voxel Animation, extension .hva) provide additional metadata for VXL voxel models,
 * including section positioning, rotation matrices, and frame-based animation data. This format enables
 * dynamic animation of voxel models through multiple frames, with each frame defining independent transformation
 * states for every model section.
 *
 * @section hva_purpose Purpose and Usage
 *
 * In Command & Conquer games, HVA files drive voxel animations during gameplay. Units animate when moving,
 * and specialized units (such as jump-jet infantry in Red Alert 2/Yuri's Revenge) animate both during
 * movement and while hovering. Notable examples include the Mammoth Mk. II (Tiberian Sun) and the Siege
 * Chopper (Yuri's Revenge).
 *
 * Each animation frame contains unique position and rotation offsets for every section defined in the
 * corresponding VXL model. By interpolating these offsets across frames, smooth animations are achieved
 * in-game.
 *
 * @warning HVA files are mandatory for VXL models. When the game engine loads a voxel file (e.g., tank.vxl),
 * it requires the corresponding HVA file (tank.hva). Missing HVA files will cause the game to crash.
 *
 * @section hva_transform Transformation Matrices
 *
 * HVA files store standard 3D transformation matrices that define how each voxel section is manipulated
 * in 3D space. These matrices support rotation, shearing, translation, and scaling operations. Each frame
 * contains a separate transformation matrix for each model section, allowing independent animation control
 * per section per frame.
 *
 * The transformation matrix structure is organized as follows:
 *
 * @code
 * | Width          | Y->X Shear     | Y->Z Shear     | X Offset |
 * | X->Y Shear     | Height         | Z->Y Shear     | Y Offset |
 * | X->Z Shear     | Y->Z Shear     | Length         | Z Offset |
 * @endcode
 *
 * Coordinate system axes:
 * - X-axis: left/right (horizontal lateral)
 * - Y-axis: up/down (vertical)
 * - Z-axis: forward/backward (depth)
 *
 * Matrix components:
 * - **Offset values**: Translation in voxel units along each axis
 * - **Width/Height/Length**: Scale multipliers for each dimension
 * - **Shear values**: Rotation components that skew one axis toward another
 *
 * @subsection hva_shearing Shear Transformations
 *
 * Shearing rotates points along one axis toward another axis. The shear effect varies based on distance
 * from the origin:
 *
 * - **Y->X Shear** (positive values): Rotates points along the Y-axis toward the X-axis. Points above
 *   the origin rotate left; points below rotate right.
 * - **X->Y Shear** (positive values): Rotates points along the X-axis toward the Y-axis. Points to the
 *   right of the origin rotate upward; points to the left rotate downward.
 * - **Z->X/Y Shear** (positive values): Rotates depth-axis points toward lateral or vertical axes,
 *   creating forward/backward tilt effects.
 *
 * Combined shear values across multiple matrix elements produce complex rotations and deformations,
 * enabling sophisticated animation effects such as treads moving, turrets rotating, or walking
 * animations for bipedal units.
 *
 * @see VXLFormat for information about the voxel model geometry
 * @ingroup Formats
 */
class HVAFormat {
protected:
	bool writeHVAHeader(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const;
	bool writeHVAFrames(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const;
	bool readHVAHeader(io::SeekableReadStream &stream, vxl::HVAHeader &header) const;
	bool readHVAFrames(io::SeekableReadStream &stream, const vxl::VXLModel &mdl, vxl::HVAModel &file) const;

public:
	bool loadHVA(const core::String &filename, const io::ArchivePtr &archive, const vxl::VXLModel &mdl, scenegraph::SceneGraph &sceneGraph);
	bool saveHVA(const core::String &filename, const io::ArchivePtr &archive, const scenegraph::SceneGraph &sceneGraph);
};

} // namespace voxelformat
