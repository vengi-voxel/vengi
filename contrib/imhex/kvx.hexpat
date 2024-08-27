#include <std/array.pat>

struct color {
  u8 b;
  u8 g;
  u8 r;
};

bitfield VisMask {
  left : 1;
  right : 1;
  front : 1;
  back : 1;
  up : 1;
  down : 1;  
  upadding : 2;
};

struct kvx {
	u8 z; //Starting z coordinate of top of slab
	u8 len; //# of bytes in the color array - slab height
	VisMask visMask; //Low 6 bits tell which of 6 faces are exposed
	std::Array<u8, len> col; //The array of colors from top to bottom
};

struct Header {
	//Total # of bytes (not including numbytes) in each mip-map level
	u32 numbytes;
	// Dimensions of voxel.
	u32 width;
	u32 depth;
	u32 height;
	//Centroid of voxel. For extra precision, this location has been shifted up by 8 bits.
	u32 pivot_x;
	u32 pivot_y;
	u32 pivot_z;
	// For compression purposes, I store the column pointers in a way that offers quick access to the data, but with slightly more overhead in calculating the positions.
	// See example of usage in voxdata.
	// NOTE: xoffset[0] = (xsiz+1)*4 + xsiz*(ysiz+1)*2  (ALWAYS)
	u32 xoffsets[width + 1];
	std::Array<u16, width * (depth + 1)> xyoffsets;
//	kvx data[voxels];
    // The last 768 bytes of the KVX file is a standard 256-color VGA palette.
    // The palette is in (Red:0, Green:1, Blue:2) order and intensities range
    // from 0-63.
//  color colors[256];
};

Header hdr @0x00;
