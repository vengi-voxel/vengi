/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/DynamicArray.h"

namespace io {
class ZipArchive;
}

namespace voxelformat {

/**
 * @brief VoxelMax (*.vmax, *.vmax.zip)
 *
 * @ingroup Formats
 */
class VMaxFormat : public PaletteFormat {
private:
	struct VMaxObject {
		core::String n;
		core::String pal;
		core::String data;
		core::String hist;
		core::String id;
		core::String pid;
		core::String t_al;
		core::String t_pa;
		core::String t_pf;
		glm::vec3 t_p{0.0f};
		glm::vec4 t_r{0.0f, 0.0f, 0.0f, 0.0f};
		glm::vec3 t_s{1.0f};
		glm::vec3 center{0.0f};
		glm::vec3 e_c{0.0f};
		glm::vec3 ind{0.0f};
		glm::vec3 e_ma{0.0f};
		glm::vec3 e_mi{0.0f};
		bool s = false;
	};

	struct VMaxScene {
		core::DynamicArray<VMaxObject> objects;
	};

	bool loadSceneJson(io::ZipArchive &zipArchive, VMaxScene &scene) const;
	bool loadObject(io::ZipArchive &archive, scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx,
					const VMaxObject &obj) const;
	bool loadPalette(io::ZipArchive &archive, const core::String &paletteName, voxel::Palette &palette,
					 const LoadContext &ctx) const;
	voxel::RawVolume *loadVolume(io::ZipArchive &archive, const LoadContext &ctx, const VMaxObject &obj) const;

	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;

public:
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override {
		return false;
	}
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream,
								   const LoadContext &ctx) override;
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
					   const LoadContext &ctx) override;
};

} // namespace voxelformat
