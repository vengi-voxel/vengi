/**
 * @file
 */

 #pragma once

 #include "voxelformat/Format.h"

 namespace voxelformat {

 /**
  * @brief Minecraft mcworld format which is a zip file that includes the @c DatFormat files
  *
  * @ingroup Formats
  */
 class MCWorldFormat : public PaletteFormat {
 protected:
	 bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
							scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
							const LoadContext &ctx) override;
	 bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					 const io::ArchivePtr &archive, const SaveContext &ctx) override {
		 return false;
	 }
 public:
	 static const io::FormatDescription &format() {
		 static io::FormatDescription f{"Minecraft mcworld", "", {"mcworld"}, {}, 0u};
		 return f;
	 }
 };

 } // namespace voxelformat
