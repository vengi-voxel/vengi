/**
 * @file
 */

 #pragma once

 #include "voxelformat/Format.h"

 namespace voxelformat {

 /**
  * @brief Minecraft bedrock leveldb format
  *
  * @ingroup Formats
  *
  * https://minecraft.fandom.com/wiki/Bedrock_Edition_level_format#Mojang_variant_LevelDB_format
  */
 class LevelDBFormat : public PaletteFormat {
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
		 static io::FormatDescription f{"Minecraft bedrock level db", {"ldb"}, {}, 0u};
		 return f;
	 }
 };

 } // namespace voxelformat
