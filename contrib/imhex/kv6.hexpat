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

struct kv6 {
	color col;
	u8 always128;
	u8 z;
	u8 always0;
	VisMask visMask;
	u8 direction;
};

struct Header {
	u32 magic;
	u32 width;
	u32 depth;
	u32 height;
	float pivot_x;
	float pivot_y;
	float pivot_z;
	u32 voxels;
	kv6 data[voxels];
	u32 xoffsets[width];
	std::Array<u16, depth * width> xyoffsets;
   // u32 palMagic;
   // color colors[256];
};

Header hdr @0x00;
