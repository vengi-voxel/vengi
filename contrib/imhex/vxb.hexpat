// sandbox vxb voxel block format
// storing 6 (or less) sides of a cube with texture data and some meta information

u32 blockSize @0x0C;

struct Image {
	u8 rgba[blockSize * blockSize * 4];
} [[hex::visualize("bitmap", rgba, blockSize, blockSize)]];

struct Material {
	u8 b;
	u8 g;
	u8 r;
	u8 a;
	u8 emissive;
};

struct Header {
	u32 magic;
	float opque;
	float emissive;
	u32 blockSize;
	u32 uniqueFaceCount;
	u32 indices[6];
	float uSpeed[6];
	float vSpeed[6];
	u32 channelAmount; // always 2
	char diffuseName[];
	char emissiveName[];

	Image diffuseImages[uniqueFaceCount];
	Image emissiveImages[uniqueFaceCount];

	u8 materialCount;
	Material materials[materialCount];
};

Header hdr @0x00;
