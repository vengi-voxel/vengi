/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {
/**
 * @brief Voxel sprite format used by the Build engine
 *
 * The KVX file format was designed to be compact, yet also renderable
 * directly from its format.  Storing a byte for every voxel would be
 * absolutely ridiculous, so I use a form of run-length encoding and store only
 * the voxels that are visible - just the surface voxels.  The "runs" are
 * stored in the ceiling to floor direction because that is the best axis to
 * use for fast rendering in the Build Engine.
 *
 * Each KVX file uses this structure for each of its mip-map levels:
 * @code
 *  long xsiz, ysiz, zsiz, xpivot, ypivot, zpivot;
 *  long xoffset[xsiz+1];
 *  short xyoffset[xsiz][ysiz+1];
 *  char rawslabdata[?];
 * @endcode
 *
 * The file can be loaded like this:
 * @code
 *  if ((fil = open("?.kvx",O_BINARY|O_RDWR,S_IREAD)) == -1) return(0);
 *  nummipmaplevels = 1;  //nummipmaplevels = 5 for unstripped KVX files
 *  for(i=0;i<nummipmaplevels;i++)
 *  {
 *   read(fil,&numbytes,4);
 *   read(fil,&xsiz,4);
 *   read(fil,&ysiz,4);
 *   read(fil,&zsiz,4);
 *   read(fil,&xpivot,4);
 *   read(fil,&ypivot,4);
 *   read(fil,&zpivot,4);
 *   read(fil,xoffset,(xsiz+1)*4);
 *   read(fil,xyoffset,xsiz*(ysiz+1)*2);
 *   read(fil,voxdata,numbytes-24-(xsiz+1)*4-xsiz*(ysiz+1)*2);
 *  }
 *  read(fil,palette,768);
 * @endcode
 *
 * @ingroup Formats
 */
class KVXFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
public:
	bool singleVolume() const override {
		return true;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Build engine", "", {"kvx"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
