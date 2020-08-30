/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "io/File.h"
#include "core/String.h"

namespace voxel {
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
 */
class KVXFormat : public VoxFileFormat {
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
