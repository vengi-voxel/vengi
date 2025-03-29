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

	for (uint32_t i = 0; i < uniqueFaces; ++i) {
		const int uniqueFace = indices[i];
		const voxel::FaceNames faceName = faceNames[uniqueFace];
		image::writeImage(diffuseImages[i], core::String::format("diffuse-r-%s.png", voxel::faceNameString(faceName)));
	}


		core::DynamicArray<core::RGBA> colors;
		colors.reserve(blockSize * blockSize);
		const voxel::FaceNames faceName = faceNames[indices[i]];
		Log::error("Save face %s", voxel::faceNameString(faceName));
		voxelutil::visitFace(*volume, faceName, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			const core::RGBA color = palette.color(voxel.getColor());
			stream->writeUInt8(color.r);
			stream->writeUInt8(color.g);
			stream->writeUInt8(color.b);
			stream->writeUInt8(color.a);
			colors.push_back(color);
		});
		core_assert(colors.size() == blockSize * blockSize);
		io::MemoryReadStream memStream(colors.data(), colors.size() * sizeof(core::RGBA));
		image::ImagePtr img = image::loadRGBAImageFromStream("diffuse", memStream, blockSize, blockSize);
		image::writeImage(img, core::String::format("diffuse-w-%s.png", voxel::faceNameString(faceName)));
