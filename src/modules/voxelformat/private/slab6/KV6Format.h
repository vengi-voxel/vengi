/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"
#include "voxelformat/Format.h"
#include "voxelformat/private/slab6/KFAFormat.h"
#include <glm/vec3.hpp>

namespace voxelformat {
/**
 * @brief Voxel sprite format used by the SLAB6 editor, voxlap and Ace of Spades
 *
 * https://github.com/vuolen/slab6-mirror/blob/master/slab6.txt
 * https://gist.github.com/falkreon/8b873ec6797ffad247375fc73614fd08
 *
 * @ingroup Formats
 */
class KV6Format : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	/**
	 * @brief Load the KV6 sprite as a single volume without applying a KFA sidecar
	 * @param[out] kenPivot Pivot in Ken / KV6 space (x, y, z)
	 */
	bool loadRawVolume(const core::String &filename, const io::ArchivePtr &archive, voxel::RawVolume *&volume,
					   palette::Palette &palette, glm::vec3 &kenPivot, const LoadContext &ctx,
					   core::Buffer<KFAFormat::KenSpriteVox> *kenVoxels = nullptr);

	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"AceOfSpades",
									  "",
									  {"kv6"},
									  {"Kvxl"},
									  VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
		return f;
	}

};

} // namespace voxelformat
