/**
 * @file
 */

#include "QBTFormat.h"
#include "core/Common.h"
#include "core/Zip.h"
#include "core/Color.h"

namespace voxel {

static const bool MergeCompounds = true;

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load qbt file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

bool QBTFormat::save(const RawVolume* volume, const io::FilePtr& file) {
	return false;
}

bool QBTFormat::skipNode(io::FileStream& stream) {
	// node type, can be ignored
	uint32_t nodeTypeId;
	wrap(stream.readInt(nodeTypeId));
	uint32_t dataSize;
	wrap(stream.readInt(dataSize));
	stream.skip(dataSize);
	return true;
}

bool QBTFormat::loadCompound(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	if (!loadMatrix(stream, volumes)) {
		return false;
	}
	uint32_t childCount;
	wrap(stream.readInt(childCount));
	for (uint32_t i = 0; i < childCount; ++i) {
		if (MergeCompounds) {
			// if you don't need the datatree you can skip child nodes
			if (!skipNode(stream)) {
				return false;
			}
		} else {
			if (!loadNode(stream, volumes)) {
				return false;
			}
		}
	}
	return true;
}

bool QBTFormat::loadMatrix(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	char buf[1024];
	uint32_t nameLength;
	wrap(stream.readInt(nameLength));
	if (nameLength >= sizeof(buf)) {
		return false;
	}
	wrap(stream.readString(nameLength, buf));
	glm::ivec3 position;
	glm::ivec3 localScale;
	glm::vec3 pivot;
	glm::ivec3 size;
	wrap(stream.readInt((uint32_t&)position.x));
	wrap(stream.readInt((uint32_t&)position.y));
	wrap(stream.readInt((uint32_t&)position.z));
	wrap(stream.readInt((uint32_t&)localScale.x));
	wrap(stream.readInt((uint32_t&)localScale.y));
	wrap(stream.readInt((uint32_t&)localScale.z));
	wrap(stream.readFloat(pivot.x));
	wrap(stream.readFloat(pivot.y));
	wrap(stream.readFloat(pivot.z));
	wrap(stream.readInt((uint32_t&)size.x));
	wrap(stream.readInt((uint32_t&)size.y));
	wrap(stream.readInt((uint32_t&)size.z));

	uint32_t voxelDataSize;
	wrap(stream.readInt(voxelDataSize));
	uint8_t* voxelData = new uint8_t[voxelDataSize];
	wrap(stream.readBuf(voxelData, voxelDataSize));

	const uint32_t voxelDataSizeDecompressed = size.x * size.y * size.z * 4;
	uint8_t* voxelDataDecompressed = new uint8_t[voxelDataSizeDecompressed];

	core::Zip z;
	if (!z.uncompress(voxelData, voxelDataSize, voxelDataDecompressed, voxelDataSizeDecompressed)) {
		Log::error("Could not load qbt file: Failed to extract zip data");
		delete [] voxelData;
		delete [] voxelDataDecompressed;
		return false;
	}
	const voxel::Region region(0, 0, 0, size.x, size.y, size.z);
	voxel::RawVolume* volume = new voxel::RawVolume(region);
	uint32_t byteCounter = 0u;
	for (int32_t x = 0; x < size.x; x++) {
		for (int32_t z = 0; z < size.z; z++) {
			for (int32_t y = 0; y < size.y; y++) {
				const uint32_t red   = ((uint32_t)voxelDataDecompressed[byteCounter++]) << 0;
				const uint32_t green = ((uint32_t)voxelDataDecompressed[byteCounter++]) << 8;
				const uint32_t blue  = ((uint32_t)voxelDataDecompressed[byteCounter++]) << 16;
				const uint32_t alpha = ((uint32_t)255) << 24;
#if 0
				const uint32_t mask  = ((uint32_t)voxelDataDecompressed[byteCounter++]);
#else
				++byteCounter;
#endif

				const glm::vec4& color = core::Color::fromRGBA(red | green | blue | alpha);
				const uint8_t index = findClosestIndex(color);
				const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
				volume->setVoxel(x, y, z, voxel);
			}
		}
	}
	delete [] voxelData;
	delete [] voxelDataDecompressed;
	volumes.push_back(volume);
	return true;
}

bool QBTFormat::loadModel(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	uint32_t childCount;
	wrap(stream.readInt(childCount));
	for (uint32_t i = 0; i < childCount; i++) {
		if (!loadNode(stream, volumes)) {
			return false;
		}
	}
	return true;
}

bool QBTFormat::loadNode(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	uint32_t nodeTypeID;
	wrap(stream.readInt(nodeTypeID));
	uint32_t dataSize;
	wrap(stream.readInt(dataSize));

	switch (nodeTypeID) {
	case 0:
		return loadMatrix(stream, volumes);
	case 1:
		return loadModel(stream, volumes);
	case 2:
		return loadCompound(stream, volumes);
	default:
		// skip node if unknown
		stream.skip(dataSize);
		break;
	}
	return true;
}

bool QBTFormat::loadFromStream(io::FileStream& stream, std::vector<RawVolume*>& volumes) {
	uint32_t header;
	wrap(stream.readInt(header))
	constexpr uint32_t headerMagic = FourCC('Q','B',' ','2');
	if (header != headerMagic) {
		Log::error("Could not load qbt file: Invalid magic found (%u vs %u)", header, headerMagic);
		return false;
	}

	uint32_t versionMajor;
	wrap(stream.readInt(versionMajor))

	uint32_t versionMinor;
	wrap(stream.readInt(versionMinor))

	glm::vec3 globalScale;
	wrap(stream.readFloat(globalScale.x));
	wrap(stream.readFloat(globalScale.y));
	wrap(stream.readFloat(globalScale.z));

	char buf[8];
	wrap(stream.readString(sizeof(buf), buf));
	if (!strncmp(buf, "COLORMAP", sizeof(buf))) {
		wrap(stream.readString(sizeof(buf), buf));

		uint32_t colorCount;
		wrap(stream.readInt(colorCount));
		_paletteSize = 0;
		_palette.reserve(colorCount);
		for (uint32_t i = 0; i < colorCount; ++i) {
			uint8_t colorByteR;
			uint8_t colorByteG;
			uint8_t colorByteB;
			uint8_t colorByteVisMask;
			wrap(stream.readByte(colorByteR));
			wrap(stream.readByte(colorByteG));
			wrap(stream.readByte(colorByteB));
			wrap(stream.readByte(colorByteVisMask));

			const uint32_t red   = ((uint32_t)colorByteR) << 24;
			const uint32_t green = ((uint32_t)colorByteG) << 16;
			const uint32_t blue  = ((uint32_t)colorByteB) << 8;
			const uint32_t alpha = ((uint32_t)255) << 0;

			const glm::vec4& color = core::Color::fromRGBA(red | green | blue | alpha);
			const uint8_t index = findClosestIndex(color);
			_palette[i] = index;
		}
		_paletteSize = colorCount;
	}

	if (strncmp(buf, "DATATREE", sizeof(buf))) {
		Log::error("Could not load qbt file: Expected to find DATATREE");
		return false;
	}

	return loadNode(stream, volumes);
}

std::vector<RawVolume*> QBTFormat::loadGroups(const io::FilePtr& file) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load qbt file: File doesn't exist");
		return std::vector<RawVolume*>();
	}
	io::FileStream stream(file.get());
	std::vector<RawVolume*> volumes;
	if (!loadFromStream(stream, volumes)) {
		return std::vector<RawVolume*>();
	}
	return volumes;
}

#undef wrap

}
