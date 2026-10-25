/**
 * @file
 */

#pragma once

#include "core/collection/Buffer.h"
#include "io/Archive.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "voxel/RawVolume.h"
#include <stdint.h>
#include <glm/vec3.hpp>

namespace voxelformat {

/**
 * @brief KWALK / Voxlap hinge animation sidecar for a KV6 sprite
 *
 * https://github.com/Ericson2314/Voxlap/blob/no-asm/share/documentation/kwalkhlp.txt
 * https://github.com/Ericson2314/Voxlap/blob/no-asm/source/kwalk.cpp
 * https://github.com/Ericson2314/Voxlap/blob/no-asm/source/voxlap5.cpp (getkfa / setlimb)
 *
 * A .kfa does not store voxels. Loaded as a same-basename sidecar when opening a
 * .kv6 (see voxformat_kv6loadkfa). KWALK splits the kv6 with floodsucksprite
 * (vis-bit z-slabs, 4-adjacent XY, z-range overlap). Hinge index i owns limb i;
 * extra hinges (numhin > numspr) are transform-only ball-joint nodes.
 * Node origin/pivot is the Ken velcro (hinge.p[1]). Seq poses are baked with
 * Ken's hinge-angle wrap-lerp so vengi TRS interpolation cannot fly limbs.
 *
 * @ingroup Formats
 */
class KFAFormat {
public:
	struct KenSpriteVox {
		uint16_t x = 0;
		uint16_t y = 0;
		uint8_t z = 0;
		uint8_t vis = 0;
	};

	struct Point3 {
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};

	struct Hinge {
		int32_t parent = -1;
		Point3 p[2]{};
		Point3 v[2]{};
		int16_t vmin = 0;
		int16_t vmax = 0;
		int8_t htype = 0;
		int8_t filler[7]{};
	};

	struct Seq {
		int32_t time = 0;
		int32_t frame = 0;
	};

	struct Data {
		core::String kv6Name;
		core::Buffer<Hinge> hinges;
		uint32_t numFrames = 0;
		core::Buffer<int16_t> frmval;
		core::Buffer<Seq> seq;
	};

	static constexpr int fps() {
		return 24;
	}

	static scenegraph::FrameIndex timeToFrame(int32_t timeMillis);
	static int32_t frameToTime(scenegraph::FrameIndex frame);

	static bool parse(const core::String &filename, const io::ArchivePtr &archive, Data &data);

	/**
	 * @brief Split the loaded KV6 volume and apply the parsed rig as scene-graph nodes + keyframes
	 * @param[in] volume The unsplit KV6 volume in vengi coordinates (caller retains ownership)
	 * @param[in] kenPivot KV6 pivot in Ken space (x, y, z)
	 * @param[in] kenVoxels KV6 voxels in file order with original vis bits (floodsucksprite)
	 */
	static bool apply(const Data &data, const voxel::RawVolume *volume, const glm::vec3 &kenPivot,
					  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					  const core::String &nodePrefix, const core::Buffer<KenSpriteVox> &kenVoxels);
};

} // namespace voxelformat
