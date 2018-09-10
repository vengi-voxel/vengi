/**
 * @file
 */

#include "VoxFormat.h"
#include "io/FileStream.h"
#include "core/Common.h"
#include "core/Color.h"
#include "core/Array.h"
#include "voxel/MaterialColor.h"

namespace voxel {

#define wrap(read) \
	if (read != 0) { \
		Log::error("Could not load vox file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return std::vector<RawVolume*>(); \
	}

bool VoxFormat::save(const RawVolume* volume, const io::FilePtr& file) {
	if (!(bool)file) {
		return false;
	}

	io::FileStream stream(file.get());
	stream.addInt(FourCC('V','O','X',' '));
	stream.addInt(150);
	stream.addInt(FourCC('M','A','I','N'));
	stream.addInt(0);
	// this is filled at the end - once we know the final size of the main chunk children
	int64_t numBytesMainChunkPos = stream.pos();
	stream.addInt(0);

	int64_t headerSize = stream.pos();
	Log::debug("headersize is: %i", (int)headerSize);

	// model size
	Log::debug("add SIZE chunk at pos %i", (int)stream.pos());
	stream.addInt(FourCC('S','I','Z','E'));
	stream.addInt(3 * sizeof(uint32_t));
	stream.addInt(0);
	const voxel::Region& region = volume->region();
	stream.addInt(region.getWidthInCells());
	stream.addInt(region.getDepthInCells());
	stream.addInt(region.getHeightInCells());

	// voxel data
	Log::debug("add XYZI chunk at pos %i", (int)stream.pos());
	stream.addInt(FourCC('X','Y','Z','I'));
	uint32_t numVoxels = 0;
	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				const voxel::Voxel& voxel = volume->voxel(x, y, z);
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				++numVoxels;
			}
		}
	}

	stream.addInt(numVoxels * 4 + sizeof(uint32_t));
	stream.addInt(0);

	stream.addInt(numVoxels);
	for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); ++y) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				const voxel::Voxel& voxel = volume->voxel(x, y, z);
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				stream.addByte(x);
				stream.addByte(z);
				stream.addByte(y);
				const uint8_t colorIndex = voxel.getColor();
				stream.addByte(colorIndex + 1);
			}
		}
	}

	Log::debug("add RGBA chunk at pos %i", (int)stream.pos());
	stream.addInt(FourCC('R','G','B','A'));
	const MaterialColorArray& materialColors = getMaterialColors();
	const int numColors = materialColors.size();
	if (numColors > 256) {
		Log::error("More colors than supported");
		return false;
	}
	stream.addInt(256 * sizeof(uint32_t));
	stream.addInt(0);

	for (int i = 0; i < numColors; ++i) {
		const uint32_t rgba = core::Color::getRGBA(materialColors[i]);
		stream.addInt(rgba);
	}
	for (int i = numColors; i < 256; ++i) {
		stream.addInt(0);
	}

	// magic, version, main chunk, main chunk size, main chunk child size
	const int64_t mainChildChunkSize = stream.pos() - headerSize;
	if (stream.seek(numBytesMainChunkPos) == -1) {
		Log::error("Failed to seek in the stream to pos %i", (int)numBytesMainChunkPos);
		return false;
	}
	stream.addInt(mainChildChunkSize);

	return true;
}

std::vector<RawVolume*> VoxFormat::loadGroups(const io::FilePtr& file) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load vox file: File doesn't exist");
		return std::vector<RawVolume*>();
	}
	io::FileStream stream(file.get());

	// 1. File Structure : RIFF style
	// -------------------------------------------------------------------------------
	// # Bytes  | Type       | Value
	// -------------------------------------------------------------------------------
	// 1x4      | char       | id 'VOX ' : 'V' 'O' 'X' 'space', 'V' is first
	// 4        | int        | version number : 150
	//
	// Chunk 'MAIN'
	// {
	//     // pack of models
	//     Chunk 'PACK'    : optional
	//
	//     // models
	//     Chunk 'SIZE'
	//     Chunk 'XYZI'
	//
	//     Chunk 'SIZE'
	//     Chunk 'XYZI'
	//
	//     ...
	//
	//     Chunk 'SIZE'
	//     Chunk 'XYZI'
	//
	//     // palette
	//     Chunk 'RGBA'    : optional
	//
	//     // materials
	//     Chunk 'MATT'    : optional
	//     Chunk 'MATT'
	//     ...
	//     Chunk 'MATT'
	// }
	// -------------------------------------------------------------------------------

	uint32_t header;
	wrap(stream.readInt(header))
	constexpr uint32_t headerMagic = FourCC('V','O','X',' ');
	if (header != headerMagic) {
		Log::error("Could not load vox file: Invalid magic found (%u vs %u)", header, headerMagic);
		return std::vector<RawVolume*>();
	}

	uint32_t version;
	wrap(stream.readInt(version))

	if (version != 150) {
		Log::warn("Vox file loading is only tested for version 150 - but we've found %i", version);
	}

	// 2. Chunk Structure
	// -------------------------------------------------------------------------------
	// # Bytes  | Type       | Value
	// -------------------------------------------------------------------------------
	// 1x4      | char       | chunk id
	// 4        | int        | num bytes of chunk content (N)
	// 4        | int        | num bytes of children chunks (M)
	// N        |            | chunk content
	// M        |            | children chunks
	// -------------------------------------------------------------------------------

	uint32_t mainChunk;
	wrap(stream.readInt(mainChunk))
	// 3. Chunk id 'MAIN' : the root chunk and parent chunk of all the other chunks
	if (mainChunk != FourCC('M','A','I','N')) {
		Log::error("Could not load vox file: Invalid magic for main chunk found");
		return std::vector<RawVolume*>();
	}

	uint32_t numBytesMainChunk;
	wrap(stream.readInt(numBytesMainChunk))
	uint32_t numBytesMainChildrenChunks;
	wrap(stream.readInt(numBytesMainChildrenChunks))

	if (stream.remaining() < numBytesMainChildrenChunks) {
		Log::error("Could not load vox file: Incomplete file");
		return std::vector<RawVolume*>();
	}

	const int64_t resetPos = stream.pos();

	RawVolume *volume = nullptr;
	uint32_t numModels = 1;

	// 8. Default Palette : if chunk 'RGBA' is absent
	// -------------------------------------------------------------------------------
	static const uint32_t palette[256] = {
		0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
		0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
		0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
		0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
		0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
		0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
		0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
		0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
		0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
		0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
		0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
		0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
		0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
		0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
		0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
		0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
	};

	const int paletteSize = lengthof(palette);
	_palette.reserve(paletteSize);
	_paletteSize = paletteSize;
	// convert to our palette
	const MaterialColorArray& materialColors = getMaterialColors();
	for (int i = 0; i < paletteSize; ++i) {
		const uint32_t p = palette[i];
		const glm::vec4& color = core::Color::fromRGBA(p);
		const int index = core::Color::getClosestMatch(color, materialColors);
		_palette[i] = index;
	}

	do {
		uint32_t chunkId;
		wrap(stream.readInt(chunkId))
		uint32_t numBytesChunk;
		wrap(stream.readInt(numBytesChunk))
		uint32_t numBytesChildrenChunks;
		wrap(stream.readInt(numBytesChildrenChunks))
		const int64_t currentChunkPos = stream.pos();
		const int64_t nextChunkPos = currentChunkPos + numBytesChunk + numBytesChildrenChunks;
		if (chunkId == FourCC('R','G','B','A')) {
			Log::debug("Found palette chunk with %u bytes (currentPos: %i, nextPos: %i)", numBytesChunk, (int)currentChunkPos, (int)nextChunkPos);
			// 7. Chunk id 'RGBA' : palette
			// -------------------------------------------------------------------------------
			// # Bytes  | Type       | Value
			// -------------------------------------------------------------------------------
			// 4 x 256  | int        | (R, G, B, A) : 1 byte for each component
			//                       | * <NOTICE>
			//                       | * color [0-254] are mapped to palette index [1-255], e.g :
			//                       |
			//                       | for ( int i = 0; i <= 254; i++ ) {
			//                       |     palette[i + 1] = ReadRGBA();
			//                       | }
			// -------------------------------------------------------------------------------
			for (int i = 0; i <= 254; i++) {
				uint32_t rgba;
				wrap(stream.readInt(rgba))
				const glm::vec4& color = core::Color::fromRGBA(rgba);
				const int index = core::Color::getClosestMatch(color, materialColors);
				Log::trace("rgba %x, r: %f, g: %f, b: %f, a: %f, index: %i, r2: %f, g2: %f, b2: %f, a2: %f",
						rgba, color.r, color.g, color.b, color.a, index, materialColors[index].r, materialColors[index].g, materialColors[index].b, materialColors[index].a);
				_palette[i + 1] = (uint8_t)index;
			}
			break;
		} else if (chunkId == FourCC('S','I','Z','E')) {
			Log::debug("Found size chunk with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)", numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
			// 5. Chunk id 'SIZE' : model size
			// -------------------------------------------------------------------------------
			// # Bytes  | Type       | Value
			// -------------------------------------------------------------------------------
			// 4        | int        | size x
			// 4        | int        | size y
			// 4        | int        | size z : gravity direction
			// -------------------------------------------------------------------------------
			// we have to flip the axis here
			uint32_t x, y, z;
			wrap(stream.readInt(x))
			wrap(stream.readInt(z))
			wrap(stream.readInt(y))
			glm::ivec3 maxsregion((int32_t)x, (int32_t)y, (int32_t)z);
			Log::debug("Found size chunk: (%u:%u:%u)", x, y, z);
			Region region(glm::ivec3(0), maxsregion);
			if (volume != nullptr) {
				delete volume;
			}
			volume = new RawVolume(region);
		}
		Log::debug("Set next chunk pos to %i of %i", (int)nextChunkPos, (int)stream.size());
		wrap(stream.seek(nextChunkPos));
	} while (stream.remaining() > 0);

	stream.seek(resetPos);

	do {
		uint32_t chunkId;
		wrap(stream.readInt(chunkId))
		uint32_t numBytesChunk;
		wrap(stream.readInt(numBytesChunk))
		uint32_t numBytesChildrenChunks;
		wrap(stream.readInt(numBytesChildrenChunks))
		const int64_t currentChunkPos = stream.pos();
		const int64_t nextChunkPos = currentChunkPos + numBytesChunk + numBytesChildrenChunks;

		if (chunkId == FourCC('P','A','C','K')) {
			Log::debug("Found pack chunk with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)", numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
			// 4. Chunk id 'PACK' : if it is absent, only one model in the file
			// -------------------------------------------------------------------------------
			// # Bytes  | Type       | Value
			// -------------------------------------------------------------------------------
			// 4        | int        | numModels : num of SIZE and XYZI chunks
			// -------------------------------------------------------------------------------
			wrap(stream.readInt(numModels))
			if (numModels > 1) {
				Log::warn("We are right now only loading the first model of a vox file");
			}
		} else if (chunkId == FourCC('X','Y','Z','I')) {
			Log::debug("Found voxel chunk with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)", numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
			// 6. Chunk id 'XYZI' : model voxels
			// -------------------------------------------------------------------------------
			// # Bytes  | Type       | Value
			// -------------------------------------------------------------------------------
			// 4        | int        | numVoxels (N)
			// 4 x N    | int        | (x, y, z, colorIndex) : 1 byte for each component
			// -------------------------------------------------------------------------------
			uint32_t numVoxels;
			wrap(stream.readInt(numVoxels))
			if (volume == nullptr) {
				Log::error("Could not load vox file: Missed SIZE chunk");
				return std::vector<RawVolume*>();
			}
			Log::debug("Found voxel chunk with %u voxels", numVoxels);
			for (uint32_t i = 0; i < numVoxels; ++i) {
				// we have to flip the axis here
				uint8_t x, y, z, colorIndex;
				wrap(stream.readByte(x))
				wrap(stream.readByte(z))
				wrap(stream.readByte(y))
				wrap(stream.readByte(colorIndex))
				const uint8_t index = convertPaletteIndex(colorIndex);
				const voxel::Voxel& voxel = voxel::createVoxel(voxel::VoxelType::Generic, index);
				volume->setVoxel(x, y, z, voxel);
			}
		} else if (chunkId == FourCC('M','A','T','T')) {
			Log::debug("Found material chunk with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)", numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
			// 9. Chunk id 'MATT' : material, if it is absent, it is diffuse material
			// -------------------------------------------------------------------------------
			// # Bytes  | Type       | Value
			// -------------------------------------------------------------------------------
			// 4        | int        | id [1-255]
			//
			// 4        | int        | material type
			//                       | 0 : diffuse
			//                       | 1 : metal
			//                       | 2 : glass
			//                       | 3 : emissive
			//
			// 4        | float      | material weight
			//                       | diffuse  : 1.0
			//                       | metal    : (0.0 - 1.0] : blend between metal and diffuse material
			//                       | glass    : (0.0 - 1.0] : blend between glass and diffuse material
			//                       | emissive : (0.0 - 1.0] : self-illuminated material
			//
			// 4        | int        | property bits : set if value is saved in next section
			//                       | bit(0) : Plastic
			//                       | bit(1) : Roughness
			//                       | bit(2) : Specular
			//                       | bit(3) : IOR
			//                       | bit(4) : Attenuation
			//                       | bit(5) : Power
			//                       | bit(6) : Glow
			//                       | bit(7) : isTotalPower (*no value)
			//
			// 4 * N    | float      | normalized property value : (0.0 - 1.0]
			//                       | * need to map to real range
			//                       | * Plastic material only accepts {0.0, 1.0} for this version
			// -------------------------------------------------------------------------------
			// TODO:
			uint32_t materialId;
			wrap(stream.readInt(materialId))
			uint32_t materialType;
			wrap(stream.readInt(materialType))
			float materialWeight;
			wrap(stream.readFloat(materialWeight))
			uint32_t materialProperties;
			wrap(stream.readInt(materialProperties))
#if 0
			for (uint32_t i = 0; i < numBytesChunk; ++i) {
				float materialPropertyValue;
				wrap(stream.readFloat(materialPropertyValue))
			}
#endif
		} else if (chunkId == FourCC('R','G','B','A') || chunkId == FourCC('S','I','Z','E')) {
			// already loaded
		} else {
			Log::warn("Unknown chunk in vox file: %u with %u bytes and %u child bytes (currentPos: %i, nextPos: %i)", chunkId, numBytesChunk, numBytesChildrenChunks, (int)currentChunkPos, (int)nextChunkPos);
		}
		Log::debug("Set next chunk pos to %i of %i", (int)nextChunkPos, (int)stream.size());
		wrap(stream.seek(nextChunkPos));
	} while (stream.remaining() > 0);

	return std::vector<RawVolume*>{volume};
}

#undef wrap

}
