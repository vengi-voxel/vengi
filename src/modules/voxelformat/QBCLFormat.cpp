/**
 * @file
 */

#include "QBCLFormat.h"
#include "core/ArrayLength.h"
#include "core/Enum.h"
#include "core/FourCC.h"
#include "core/Zip.h"
#include "core/Color.h"
#include "core/Assert.h"
#include "core/Log.h"

namespace voxel {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load qbcl file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Could not load qbcl file: Not enough data in stream " CORE_STRINGIFY(read) " - still %i bytes left", (int)stream.remaining()); \
		return false; \
	}

bool QBCLFormat::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	return false;
}

#if 0
static bool readString(io::FileStream &stream, core::String &dest) {
	uint32_t size;
	wrap(stream.readInt(size))
	if (size > 1024) {
		Log::warn("Found big string - ignore it");
		stream.skip(size);
		return false;
	}
	dest.reserve(size);
	for (uint32_t i = 0; i < size; ++i) {
		uint8_t chr;
		wrap(stream.readByte(chr))
		dest.append(chr);
	}
	return true;
}
#endif

bool QBCLFormat::readMatrix(io::FileStream &stream) {
	uint32_t mx; // 32
	uint32_t my;
	uint32_t mz;
	wrap(stream.readInt(mx))
	wrap(stream.readInt(my))
	wrap(stream.readInt(mz))
	Log::debug("QBCL: matrix size %u:%u:%u", mx, my, mz);
	// TODO
	return false;
}

bool QBCLFormat::readModel(io::FileStream &stream) {
	uint32_t x;
	uint32_t y;
	uint32_t z;

	wrap(stream.readInt(x))
	wrap(stream.readInt(y))
	wrap(stream.readInt(z))
	Log::debug("QBCL: model size %u:%u:%u", x, y, z);

	uint32_t r[6];
	for (int i = 0; i < lengthof(r); ++i) {
		wrap(stream.readInt(r[i]))
		Log::debug("QBCL: r[%i] = %u", i, r[i]);
	}
	uint32_t n;
	uint32_t u;
	uint32_t v;

	wrap(stream.readInt(n))
	wrap(stream.readInt(u))
	wrap(stream.readInt(v))
	Log::debug("QBCL: n: %u, u: %u, v: %u", n, u, v);
	// TODO
	return false;
}

bool QBCLFormat::readCompound(io::FileStream &stream) {
	return false;
}

bool QBCLFormat::loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) {
	if (!(bool)file || !file->exists()) {
		Log::error("Could not load qb file: File doesn't exist");
		return false;
	}
	io::FileStream stream(file.get());
	uint32_t magic;
	wrap(stream.readInt(magic))
	if (magic != FourCC('Q', 'B', 'C', 'L')) {
		Log::error("Invalid magic found - no qbcl file");
		return false;
	}
	wrap(stream.readInt(_version))
	uint32_t flags;
	wrap(stream.readInt(flags))
	uint32_t thumbWidth;
	wrap(stream.readInt(thumbWidth))
	uint32_t thumbHeight;
	wrap(stream.readInt(thumbHeight))
	wrapBool(stream.skip(thumbWidth * thumbHeight * 4))

#if 0
	core::String title;
	wrapBool(readString(stream, title))
	core::String desc;
	wrapBool(readString(stream, desc))
	core::String metadata;
	wrapBool(readString(stream, metadata))
	core::String author;
	wrapBool(readString(stream, author))
	core::String company;
	wrapBool(readString(stream, company))
	core::String website;
	wrapBool(readString(stream, website))
	core::String copyright;
	wrapBool(readString(stream, copyright))
	uint8_t guid[16];
	wrap(stream.readBuf(guid, lengthof(guid)))

	uint32_t unknown;
	stream.readInt(unknown);
	Log::debug("QBCL: unknown value %u at offset %u", unknown, (uint32_t)stream.pos());
	stream.readInt(unknown);
	Log::debug("QBCL: unknown value %u at offset %u", unknown, (uint32_t)stream.pos());

	while (stream.remaining() > 0) {
		core::String type;
		wrapBool(readString(stream, type))
		uint8_t unknownByte;
		wrap(stream.readByte(unknownByte))
		Log::debug("QBCL: unknown value %u at offset %u", unknownByte, (uint32_t)stream.pos());
		wrap(stream.readByte(unknownByte))
		Log::debug("QBCL: unknown value %u at offset %u", unknownByte, (uint32_t)stream.pos());
		wrap(stream.readByte(unknownByte))
		Log::debug("QBCL: unknown value %u at offset %u", unknownByte, (uint32_t)stream.pos());
		if (type == "Model") {
			wrapBool(readModel(stream))
		} else if (type == "Matrix") {
			wrapBool(readMatrix(stream))
		} else if (type == "Compound") {
			wrapBool(readCompound(stream))
		} else {
			Log::warn("Unknown type found: '%s'", type.c_str());
			return false;
		}
	}
#endif

	return false;
}

}

#undef wrap
#undef wrapBool
