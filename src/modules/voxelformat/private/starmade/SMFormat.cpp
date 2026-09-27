/**
 * @file
 */

#include "SMFormat.h"
#include "color/Color.h"
#include "color/ColorUtil.h"
#include "core/ArrayLength.h"
#include "core/Bits.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "io/ZipReadStream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// https://starmadepedia.net/wiki/ID_list
#include "SMPalette.h"

namespace voxelformat {

namespace priv {
constexpr int segments = 16;
constexpr int maxSegments = segments * segments * segments;

// SMD2: 16x16x16 blocks per chunk, 5120 bytes per chunk slot
constexpr int smd2Blocks = 16;
constexpr int smd2ChunkDataSize = 5120;
constexpr int smd2SegmentHeaderSize = 25; // 8 timestamp + 12 position + 4 dataLength + 1 type

// SMD3: 32x32x32 blocks per segment
constexpr int smd3Blocks = 32;
constexpr int smd3SegmentHeaderSize =
	26; // 1 segmentVersion + 8 timestamp + 12 position + 1 hasValidData + 4 compressedSize
constexpr int smd3SegmentDataSize = ((smd3Blocks * smd3Blocks * smd3Blocks) * 3 / 2) - smd3SegmentHeaderSize;

} // namespace priv

static bool readIvec3(io::SeekableReadStream &stream, glm::ivec3 &v);

// StarMade Tag types (org.schema.schine.resource.tag.Tag)
enum SMTagType {
	SMTag_Finish = 0,
	SMTag_Byte = 1,
	SMTag_Short = 2,
	SMTag_Int = 3,
	SMTag_Long = 4,
	SMTag_Float = 5,
	SMTag_Double = 6,
	SMTag_ByteArray = 7,
	SMTag_String = 8,
	SMTag_Vec3f = 9,
	SMTag_Vec3i = 10,
	SMTag_Vec3b = 11,
	SMTag_List = 12,
	SMTag_Struct = 13,
	SMTag_Serializable = 14,
	SMTag_Vec4f = 15,
	SMTag_Mat4 = 16,
	SMTag_Nothing = 17,
	SMTag_Mat3 = 18
};

struct SMDock {
	core::String name;
	glm::vec3 translation{0.0f};
	glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};
};

static void stripTrailingSlash(core::String &path) {
	while (!path.empty() && path.last() == '/') {
		path = path.substr(0, path.size() - 1);
	}
}

static core::String pathBasename(const core::String &path) {
	core::String p = path;
	stripTrailingSlash(p);
	const size_t slash = p.rfind('/');
	if (slash == core::String::npos) {
		return p;
	}
	return p.substr(slash + 1);
}

// DATA/file.smd3 -> empty, ship/DATA/file.smd3 -> ship
static core::String entityPrefixFromDir(const core::String &dir) {
	core::String prefix = dir;
	if (core::string::endsWith(prefix, "DATA/")) {
		prefix = prefix.substr(0, prefix.size() - 5);
	} else if (core::string::endsWith(prefix, "DATA")) {
		prefix = prefix.substr(0, prefix.size() - 4);
	}
	stripTrailingSlash(prefix);
	return prefix;
}

static glm::mat3 mat4RowMajorBasis(const float m[16]) {
	return glm::mat3(m[0], m[4], m[8], m[1], m[5], m[9], m[2], m[6], m[10]);
}

// javax.vecmath Matrix3f.rotX/Y/Z (column-vector multiply)
static glm::mat3 smRotX(float a) {
	const float c = glm::cos(a);
	const float s = glm::sin(a);
	return glm::mat3(1.0f, 0.0f, 0.0f, 0.0f, c, s, 0.0f, -s, c);
}

static glm::mat3 smRotY(float a) {
	const float c = glm::cos(a);
	const float s = glm::sin(a);
	return glm::mat3(c, 0.0f, -s, 0.0f, 1.0f, 0.0f, s, 0.0f, c);
}

static glm::mat3 smRotZ(float a) {
	const float c = glm::cos(a);
	const float s = glm::sin(a);
	return glm::mat3(c, s, 0.0f, -s, c, 0.0f, 0.0f, 0.0f, 1.0f);
}

struct SMRigid {
	glm::mat3 basis{1.0f};
	glm::vec3 origin{0.0f};
};

static SMRigid smMul(const SMRigid &a, const SMRigid &b) {
	SMRigid out;
	out.basis = a.basis * b.basis;
	out.origin = a.basis * b.origin + a.origin;
	return out;
}

static SMRigid smInverse(const SMRigid &t) {
	SMRigid out;
	out.basis = glm::transpose(t.basis);
	out.origin = -out.basis * t.origin;
	return out;
}

static const int8_t smOriencubeMirror[24] = {4,  5,	 6,	 7,	 0,	 1,	 2,	 3,	 12, 13, 14, 15,
											 8,	 9,	 10, 11, 20, 21, 22, 23, 16, 17, 18, 19};
// Element side: 0 front +Z, 1 back -Z, 2 top +Y, 3 bottom -Y, 4 right -X, 5 left +X
static const int8_t smOriencubePrimary[24] = {0, 0, 0, 0, 1, 1, 1, 1, 3, 3, 3, 3,
											  2, 2, 2, 2, 4, 4, 4, 4, 5, 5, 5, 5};
static const float smOriencubeSecondaryY[24] = {
	0.0f, 1.5707964f, 3.1415927f, 4.712389f, 3.1415927f, 1.5707964f, 0.0f,	  4.712389f,
	3.1415927f, 4.712389f, 0.0f, 1.5707964f, 3.1415927f, 1.5707964f, 0.0f,	  4.712389f,
	0.0f, 1.5707964f, 3.1415927f, 4.712389f, 0.0f,		  4.712389f,  3.1415927f, 1.5707964f};

static SMRigid oriencubeGetTrans(const glm::ivec3 &position, int orientation, bool mirrored, int move) {
	int idx = orientation % 24;
	if (idx < 0) {
		idx += 24;
	}
	if (mirrored) {
		idx = (int)smOriencubeMirror[idx];
	}
	const int face = (int)smOriencubePrimary[idx];
	glm::vec3 origin((float)(position.x - 16), (float)(position.y - 16), (float)(position.z - 16));
	glm::mat3 primary(1.0f);
	if (face == 0) {
		primary = smRotX(glm::half_pi<float>());
		origin.z += (float)move;
	} else if (face == 1) {
		primary = smRotX(-glm::half_pi<float>());
		origin.z -= (float)move;
	} else if (face == 3) {
		primary = smRotZ(-glm::pi<float>());
		origin.y -= (float)move;
	} else if (face == 2) {
		origin.y += (float)move;
	} else if (face == 4) {
		primary = smRotZ(glm::half_pi<float>());
		origin.x -= (float)move;
	} else {
		primary = smRotZ(-glm::half_pi<float>());
		origin.x += (float)move;
	}
	SMRigid trans;
	trans.basis = primary * smRotY(smOriencubeSecondaryY[idx]);
	trans.origin = origin;
	return trans;
}

// RailRelation.getBlockTransform (static dock / originalOut path)
static SMRigid railBlockTransform(const glm::ivec3 &railPos, int railOri, const glm::ivec3 &dockedPos, int dockedOri,
								  const glm::mat3 &movingBasis, const glm::vec3 &movingOrigin) {
	SMRigid rail = oriencubeGetTrans(railPos, railOri, false, 1);
	rail.basis = movingBasis * rail.basis;
	SMRigid docked = oriencubeGetTrans(dockedPos, dockedOri, true, 0);
	SMRigid moving;
	moving.basis = glm::mat3(1.0f);
	moving.origin = movingOrigin;
	return smMul(smMul(moving, rail), smInverse(docked));
}

static int smTagAbs(int8_t type) {
	return type < 0 ? -(int)type : (int)type;
}

static bool skipSMPayload(io::SeekableReadStream &stream, int type, int depth);

static bool readSMTagHeader(io::SeekableReadStream &stream, int8_t &type, core::String &name) {
	if (stream.readInt8(type) != 0) {
		return false;
	}
	name = core::String::Empty;
	if (type > 0) {
		if (!stream.readPascalStringUInt16BE(name)) {
			return false;
		}
	}
	return true;
}

static bool skipSMPayload(io::SeekableReadStream &stream, int type, int depth) {
	if (depth > 32) {
		Log::error("StarMade tag nesting too deep");
		return false;
	}
	switch (type) {
	case SMTag_Finish:
	case SMTag_Nothing:
		return true;
	case SMTag_Byte:
		return stream.skip(1) != -1;
	case SMTag_Short:
		return stream.skip(2) != -1;
	case SMTag_Int:
	case SMTag_Float:
		return stream.skip(4) != -1;
	case SMTag_Long:
	case SMTag_Double:
	case SMTag_Vec4f:
		return stream.skip(8) != -1;
	case SMTag_Vec3f:
	case SMTag_Vec3i:
		return stream.skip(12) != -1;
	case SMTag_Vec3b:
		return stream.skip(3) != -1;
	case SMTag_Mat3:
		return stream.skip(36) != -1;
	case SMTag_Mat4:
		return stream.skip(64) != -1;
	case SMTag_ByteArray: {
		int32_t len = 0;
		if (stream.readInt32BE(len) != 0 || len < 0) {
			return false;
		}
		return stream.skip(len) != -1;
	}
	case SMTag_String: {
		core::String dummy;
		return stream.readPascalStringUInt16BE(dummy);
	}
	case SMTag_List: {
		int8_t listType = 0;
		int32_t count = 0;
		if (stream.readInt8(listType) != 0 || stream.readInt32BE(count) != 0 || count < 0 || count > 1000000) {
			return false;
		}
		const int absType = smTagAbs(listType);
		for (int32_t i = 0; i < count; ++i) {
			if (!skipSMPayload(stream, absType, depth + 1)) {
				return false;
			}
		}
		return true;
	}
	case SMTag_Struct: {
		for (;;) {
			int8_t childType = 0;
			core::String childName;
			if (!readSMTagHeader(stream, childType, childName)) {
				return false;
			}
			if (childType == SMTag_Finish) {
				return true;
			}
			if (!skipSMPayload(stream, smTagAbs(childType), depth + 1)) {
				return false;
			}
		}
	}
	case SMTag_Serializable:
		Log::debug("Skipping StarMade SERIALIZABLE tag");
		return false;
	default:
		Log::error("Unknown StarMade tag type %i", type);
		return false;
	}
}

struct SMRailPiece {
	glm::ivec3 position{0};
	int orientation = 0;
	bool valid = false;
};

static bool skipUntilFinish(io::SeekableReadStream &stream, int depth) {
	for (;;) {
		int8_t type = 0;
		core::String name;
		if (!readSMTagHeader(stream, type, name)) {
			return false;
		}
		if (type == SMTag_Finish) {
			return true;
		}
		if (!skipSMPayload(stream, smTagAbs(type), depth + 1)) {
			return false;
		}
	}
}

static bool expectSMTag(io::SeekableReadStream &stream, int expectedAbs, int depth) {
	int8_t type = 0;
	core::String name;
	if (!readSMTagHeader(stream, type, name)) {
		return false;
	}
	if (smTagAbs(type) != expectedAbs) {
		if (type != SMTag_Finish && !skipSMPayload(stream, smTagAbs(type), depth + 1)) {
			return false;
		}
		return false;
	}
	return true;
}

static bool readSMMat4(io::SeekableReadStream &stream, float m[16], int depth) {
	if (!expectSMTag(stream, SMTag_Mat4, depth)) {
		return false;
	}
	for (int i = 0; i < 16; ++i) {
		if (stream.readFloatBE(m[i]) != 0) {
			return false;
		}
	}
	return true;
}

static bool parseRailPiece(io::SeekableReadStream &stream, SMRailPiece &piece, int depth) {
	if (!expectSMTag(stream, SMTag_Struct, depth)) {
		return false;
	}
	int8_t kind = 0;
	if (!expectSMTag(stream, SMTag_Byte, depth + 1) || stream.readInt8(kind) != 0) {
		return false;
	}
	(void)kind;
	if (!expectSMTag(stream, SMTag_String, depth + 1)) {
		return false;
	}
	core::String uid;
	if (!stream.readPascalStringUInt16BE(uid)) {
		return false;
	}
	if (!expectSMTag(stream, SMTag_Vec3i, depth + 1) || !readIvec3(stream, piece.position)) {
		return false;
	}
	if (!expectSMTag(stream, SMTag_Short, depth + 1)) {
		return false;
	}
	int16_t blockType = 0;
	if (stream.readInt16BE(blockType) != 0) {
		return false;
	}
	int8_t orient = 0;
	if (!expectSMTag(stream, SMTag_Byte, depth + 1) || stream.readInt8(orient) != 0) {
		return false;
	}
	piece.orientation = (int)orient;
	piece.valid = true;
	return skipUntilFinish(stream, depth + 1);
}

static bool parseRailChildTag(io::SeekableReadStream &stream, SMDock &dock) {
	int16_t tagVersion = 0;
	if (stream.readInt16BE(tagVersion) != 0) {
		return false;
	}

	if (!expectSMTag(stream, SMTag_Struct, 0)) {
		return false;
	}

	if (!expectSMTag(stream, SMTag_Byte, 1)) {
		return skipUntilFinish(stream, 0);
	}
	int8_t railTagType = 0;
	if (stream.readInt8(railTagType) != 0) {
		return false;
	}
	if (railTagType != 1 && railTagType != 3) {
		return skipUntilFinish(stream, 0);
	}

	if (!expectSMTag(stream, SMTag_Struct, 1)) {
		return skipUntilFinish(stream, 0);
	}
	if (!expectSMTag(stream, SMTag_Struct, 2)) {
		return skipUntilFinish(stream, 0);
	}

	SMRailPiece rail;
	SMRailPiece docked;
	if (!parseRailPiece(stream, rail, 3) || !parseRailPiece(stream, docked, 3)) {
		return false;
	}

	float railTransform[16];
	float dockedTransform[16];
	float moving[16];
	if (!readSMMat4(stream, railTransform, 3) || !readSMMat4(stream, dockedTransform, 3)) {
		return false;
	}
	if (!expectSMTag(stream, SMTag_Vec3i, 3)) {
		return false;
	}
	glm::ivec3 railContact;
	if (!readIvec3(stream, railContact)) {
		return false;
	}
	if (!readSMMat4(stream, moving, 3)) {
		return false;
	}
	if (!skipUntilFinish(stream, 3) || !skipUntilFinish(stream, 2) || !skipUntilFinish(stream, 1)) {
		return false;
	}

	if (!rail.valid || !docked.valid) {
		return true;
	}

	(void)railTransform;
	(void)dockedTransform;
	(void)railContact;

	const glm::vec3 movingT(moving[3], moving[7], moving[11]);
	const SMRigid dockedToParent =
		railBlockTransform(rail.position, rail.orientation, docked.position, docked.orientation,
						   mat4RowMajorBasis(moving), movingT);
	dock.translation = dockedToParent.origin;
	dock.orientation = glm::normalize(glm::quat_cast(dockedToParent.basis));
	Log::debug("Rail dock %s offset=(%f,%f,%f) rail=(%i,%i,%i) ori=%i docker=(%i,%i,%i) ori=%i", dock.name.c_str(),
			   dock.translation.x, dock.translation.y, dock.translation.z, rail.position.x, rail.position.y,
			   rail.position.z, rail.orientation, docked.position.x, docked.position.y, docked.position.z,
			   docked.orientation);
	return true;
}

static bool skipWirelessAndReadRailChildren(io::SeekableReadStream &stream, int32_t metaVersion,
											core::DynamicArray<SMDock> &docks) {
	if (metaVersion >= 2) {
		core::String railUid;
		if (!stream.readPascalStringUInt16BE(railUid)) {
			return false;
		}
		int32_t wirelessCount = 0;
		if (stream.readInt32BE(wirelessCount) != 0 || wirelessCount < 0 || wirelessCount > 100000) {
			return false;
		}
		for (int32_t i = 0; i < wirelessCount; ++i) {
			core::String marking;
			if (!stream.readPascalStringUInt16BE(marking)) {
				return false;
			}
			int64_t dummy = 0;
			if (stream.readInt64BE(dummy) != 0 || stream.readInt64BE(dummy) != 0) {
				return false;
			}
		}
	}
	int32_t childCount = 0;
	if (stream.readInt32BE(childCount) != 0 || childCount < 0 || childCount > 10000) {
		return false;
	}
	for (int32_t i = 0; i < childCount; ++i) {
		core::String childPath;
		if (!stream.readPascalStringUInt16BE(childPath)) {
			return false;
		}
		int32_t tagSize = 0;
		if (stream.readInt32BE(tagSize) != 0 || tagSize < 0) {
			return false;
		}
		SMDock dock;
		dock.name = pathBasename(childPath);
		if (tagSize == 0) {
			docks.push_back(dock);
			continue;
		}
		const int64_t tagStart = stream.pos();
		if (!parseRailChildTag(stream, dock)) {
			Log::warn("Failed to parse rail child tag for %s", dock.name.c_str());
			if (stream.seek(tagStart + tagSize) == -1) {
				return false;
			}
		} else {
			const int64_t consumed = stream.pos() - tagStart;
			if (consumed < tagSize) {
				if (stream.skip((int64_t)tagSize - consumed) == -1) {
					return false;
				}
			}
		}
		docks.push_back(dock);
	}
	return true;
}

static bool parseSmbpm(io::SeekableReadStream &stream, core::DynamicArray<SMDock> &docks) {
	int32_t metaVersion = 0;
	if (stream.readInt32BE(metaVersion) != 0) {
		return false;
	}
	Log::debug("StarMade meta version %i", (int)metaVersion);
	// docking offset is always block pos - 16
	const int coreOffset = 16;

	while (stream.remaining() > 0) {
		int8_t tag = 0;
		if (stream.readInt8(tag) != 0) {
			return false;
		}
		if (tag == 1) {
			break;
		}
		if (tag == 2 || tag == 9) {
			// manager / thrust consume the rest of the file
			break;
		}
		if (tag == 3) {
			int32_t count = 0;
			if (stream.readInt32BE(count) != 0 || count < 0 || count > 10000) {
				return false;
			}
			for (int32_t i = 0; i < count; ++i) {
				core::String childPath;
				if (!stream.readPascalStringUInt16BE(childPath)) {
					return false;
				}
				glm::ivec3 pos;
				if (!readIvec3(stream, pos)) {
					return false;
				}
				float sizeX, sizeY, sizeZ;
				if (stream.readFloatBE(sizeX) != 0 || stream.readFloatBE(sizeY) != 0 || stream.readFloatBE(sizeZ) != 0) {
					return false;
				}
				int16_t style = 0;
				int8_t orientation = 0;
				if (stream.readInt16BE(style) != 0 || stream.readInt8(orientation) != 0) {
					return false;
				}
				SMDock dock;
				dock.name = pathBasename(childPath);
				dock.translation = glm::vec3((float)(pos.x - coreOffset), (float)(pos.y - coreOffset),
											 (float)(pos.z - coreOffset));
				Log::debug("Docking entry %s pos=(%i,%i,%i) orient=%i", dock.name.c_str(), pos.x, pos.y, pos.z,
						   (int)orientation);
				docks.push_back(dock);
			}
			continue;
		}
		if (tag == 4) {
			float dummy = 0.0f;
			for (int i = 0; i < 6; ++i) {
				if (stream.readFloatBE(dummy) != 0) {
					return false;
				}
			}
			if (!skipWirelessAndReadRailChildren(stream, metaVersion, docks)) {
				return false;
			}
			continue;
		}
		if (tag == 5) {
			int32_t tagSize = 0;
			if (stream.readInt32BE(tagSize) != 0 || tagSize < 0) {
				return false;
			}
			if (stream.skip(tagSize) == -1) {
				return false;
			}
			continue;
		}
		if (tag == 6) {
			int8_t exists = 0;
			if (stream.readInt8(exists) != 0) {
				return false;
			}
			if (exists > 0) {
				int32_t count = 0;
				if (stream.readInt32BE(count) != 0 || count < 0 || count > 100000) {
					return false;
				}
				// 3*int32 + int16 + 3 bytes
				if (stream.skip((int64_t)count * 17) == -1) {
					return false;
				}
			}
			continue;
		}
		if (tag == 7 || tag == 8) {
			int8_t exists = 0;
			if (stream.readInt8(exists) != 0) {
				return false;
			}
			if (exists > 0) {
				int32_t count = 0;
				if (stream.readInt32BE(count) != 0 || count < 0 || count > 100000) {
					return false;
				}
				if (stream.skip((int64_t)count * 16) == -1) {
					return false;
				}
			}
			continue;
		}
		Log::debug("Unknown StarMade meta tag %i", (int)tag);
		break;
	}
	return true;
}

static const SMDock *findDock(const core::DynamicArray<SMDock> &docks, const core::String &attachedName) {
	const core::String wanted = pathBasename(attachedName);
	for (const SMDock &dock : docks) {
		if (dock.name == wanted) {
			return &dock;
		}
	}
	return nullptr;
}

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

static bool readIvec3(io::SeekableReadStream &stream, glm::ivec3 &v) {
	if (stream.readInt32BE(v.x) == -1) {
		Log::error("failed to read int vector x component");
		return false;
	}
	if (stream.readInt32BE(v.y) == -1) {
		Log::error("failed to read int vector y component");
		return false;
	}
	if (stream.readInt32BE(v.z) == -1) {
		Log::error("failed to read int vector z component");
		return false;
	}
	return true;
}

bool SMFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
							  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							  const LoadContext &ctx) {
	ctx.setProgress(0.0f);
	core::Map<int, int> blockPal;
	for (int i = 0; i < lengthof(BLOCKCOLOR); ++i) {
		blockPal.put(BLOCKCOLOR[i].blockId, palette.getClosestMatch(BLOCKCOLOR[i].color));
	}
	const core::String &extension = core::string::extractExtension(filename);
	if (extension == "smd3") {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			Log::error("Could not load file %s", filename.c_str());
			return false;
		}
		const bool loaded = readSmd3(*stream, sceneGraph, blockPal, {0, 0, 0}, palette, 0);
		if (loaded) {
			ctx.setProgress(1.0f);
		}
		return loaded;
	} else if (extension == "smd2") {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			Log::error("Could not load file %s", filename.c_str());
			return false;
		}
		const bool loaded = readSmd2(*stream, sceneGraph, blockPal, {0, 0, 0}, palette, 0);
		if (loaded) {
			ctx.setProgress(1.0f);
		}
		return loaded;
	} else if (extension == "sment") {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			Log::error("Could not load file %s", filename.c_str());
			return false;
		}
		io::ArchivePtr zipArchive = io::openZipArchive(stream);
		io::ArchiveFiles files;
		zipArchive->list("*.smd3,*.smd2", files);
		if (files.empty()) {
			Log::error("No smd3 or smd2 files found in %s", filename.c_str());
			return false;
		}
		io::ArchiveFiles metaFiles;
		zipArchive->list("*.smbpm", metaFiles);

		core::String rootPrefix;
		for (const io::FilesystemEntry &m : metaFiles) {
			if (!m.fullPath.contains("ATTACHED_")) {
				rootPrefix = entityPrefixFromDir(core::string::extractDir(m.fullPath));
				break;
			}
		}
		if (rootPrefix.empty()) {
			for (const io::FilesystemEntry &e : files) {
				if (!e.fullPath.contains("ATTACHED_")) {
					rootPrefix = entityPrefixFromDir(core::string::extractDir(e.fullPath));
					break;
				}
			}
		}

		auto loadEntity = [&](const core::String &prefix, int parent, const SMDock *dock,
							  auto &&loadEntityRef) -> bool {
			scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group);
			const core::String groupName = prefix.empty() ? core::string::extractFilename(filename) : pathBasename(prefix);
			group.setName(groupName);
			if (dock != nullptr) {
				group.setTranslation(dock->translation);
				group.setRotation(dock->orientation);
			}
			const int groupId = sceneGraph.emplace(core::move(group), parent);
			if (groupId == InvalidNodeId) {
				return false;
			}

			core::DynamicArray<SMDock> childDocks;
			const core::String metaPath =
				prefix.empty() ? core::String("meta.smbpm") : core::String::format("%s/meta.smbpm", prefix.c_str());
			{
				core::ScopedPtr<io::SeekableReadStream> metaStream(zipArchive->readStream(metaPath));
				if (metaStream) {
					if (!parseSmbpm(*metaStream, childDocks)) {
						Log::warn("Failed to parse %s", metaPath.c_str());
					}
				}
			}

			int fileIndex = 0;
			for (const io::FilesystemEntry &e : files) {
				ctx.report("segment", fileIndex++, (int)files.size());
				const core::String dataPrefix =
					prefix.empty() ? core::String("DATA/") : core::String::format("%s/DATA/", prefix.c_str());
				if (!core::string::startsWith(e.fullPath, dataPrefix)) {
					continue;
				}
				const core::String afterData = e.fullPath.substr(dataPrefix.size());
				if (afterData.contains("/")) {
					continue;
				}
				const core::String &fileExt = core::string::extractExtension(e.name);
				const bool isSmd3 = fileExt == "smd3";
				const bool isSmd2 = fileExt == "smd2";
				if (!isSmd2 && !isSmd3) {
					continue;
				}
				glm::ivec3 position(0);
				core::DynamicArray<core::String> parts;
				core::string::splitString(e.name, parts, ".");
				const int l = (int)parts.size();
				if (l >= 4) {
					position.x = core::string::toInt(parts[l - 4]) * priv::segments;
					position.y = core::string::toInt(parts[l - 3]) * priv::segments;
					position.z = core::string::toInt(parts[l - 2]) * priv::segments;
				}
				core::ScopedPtr<io::SeekableReadStream> modelStream(zipArchive->readStream(e.fullPath));
				if (!modelStream) {
					Log::warn("Failed to load zip archive entry %s", e.fullPath.c_str());
					continue;
				}
				if (isSmd3) {
					if (!readSmd3(*modelStream, sceneGraph, blockPal, position, palette, groupId)) {
						Log::warn("Failed to load %s from %s", e.fullPath.c_str(), filename.c_str());
					}
				} else if (!readSmd2(*modelStream, sceneGraph, blockPal, position, palette, groupId)) {
					Log::warn("Failed to load %s from %s", e.fullPath.c_str(), filename.c_str());
				}
			}

			core::DynamicArray<core::String> childNames;
			const core::String attachedPrefix =
				prefix.empty() ? core::String("ATTACHED_") : core::String::format("%s/ATTACHED_", prefix.c_str());
			for (const io::FilesystemEntry &e : files) {
				if (!core::string::startsWith(e.fullPath, attachedPrefix)) {
					continue;
				}
				core::String rest = e.fullPath.substr(attachedPrefix.size());
				const size_t slash = rest.find("/");
				if (slash == core::String::npos) {
					continue;
				}
				const core::String childName = core::String("ATTACHED_") + rest.substr(0, slash);
				bool exists = false;
				for (const core::String &n : childNames) {
					if (n == childName) {
						exists = true;
						break;
					}
				}
				if (!exists) {
					childNames.push_back(childName);
				}
			}
			for (const core::String &childName : childNames) {
				const core::String childPrefix =
					prefix.empty() ? childName : core::String::format("%s/%s", prefix.c_str(), childName.c_str());
				const SMDock *childDock = findDock(childDocks, childName);
				if (!loadEntityRef(childPrefix, groupId, childDock, loadEntityRef)) {
					Log::warn("Failed to load attached entity %s", childPrefix.c_str());
				}
			}
			return true;
		};

		if (!loadEntity(rootPrefix, 0, nullptr, loadEntity)) {
			Log::error("Failed to load StarMade blueprint entities from %s", filename.c_str());
		}
		ctx.report("segment", (int)files.size(), (int)files.size());
	}
	const bool loaded = !sceneGraph.empty();
	if (loaded) {
		ctx.setProgress(1.0f);
	}
	return loaded;
}

bool SMFormat::readSmd2(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						const core::Map<int, int> &blockPal, const glm::ivec3 &position,
						const palette::Palette &palette, int parent) {
	uint32_t version;
	wrap(stream.readUInt32BE(version))

	// SMD2 segment index: 16*16*16 entries of (int32 offset, int32 size)
	for (int i = 0; i < priv::maxSegments; i++) {
		int32_t segmentOffset;
		wrap(stream.readInt32BE(segmentOffset))
		int32_t segmentSize;
		wrap(stream.readInt32BE(segmentSize))
	}
	// SMD2 timestamp table: 16*16*16 entries of int64
	for (int i = 0; i < priv::maxSegments; i++) {
		uint64_t timestamp;
		wrap(stream.readUInt64BE(timestamp))
	}
	int segmentIndex = 0;
	while (!stream.eos()) {
		if (!readSegment(stream, sceneGraph, blockPal, version, 2, palette, parent,
						 core::String::format("smd2_%i", segmentIndex++))) {
			Log::error("Failed to read segment");
			return false;
		}
	}

	return true;
}

bool SMFormat::readSmd3(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						const core::Map<int, int> &blockPal, const glm::ivec3 &position,
						const palette::Palette &palette, int parent) {
	uint32_t version;
	wrap(stream.readUInt32BE(version))

	core::Map<uint16_t, uint16_t> segmentsMap;

	for (int i = 0; i < priv::maxSegments; i++) {
		uint16_t segmentId;
		wrap(stream.readUInt16BE(segmentId))
		uint16_t segmentSize;
		wrap(stream.readUInt16BE(segmentSize))
		if (segmentId > 0) {
			Log::debug("segment %i with size: %i", (int)segmentId, (int)segmentSize);
			segmentsMap.put(segmentId, segmentSize);
		}
	}
	int segmentIndex = 0;
	while (!stream.eos()) {
		if (!readSegment(stream, sceneGraph, blockPal, version, 3, palette, parent,
						 core::String::format("smd3_%i", segmentIndex++))) {
			Log::error("Failed to read segment");
			return false;
		}
	}

	return true;
}

static glm::ivec3 posByIndex(uint32_t blockIndex, int blocks) {
	const int planeBlocks = blocks * blocks;
	const int z = (int)blockIndex / planeBlocks;
	const int divR = (int)blockIndex % planeBlocks;
	const int y = divR / blocks;
	const int x = divR % blocks;
	return glm::ivec3(x, y, z);
}

size_t SMFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							 const LoadContext &ctx) {
	for (int i = 0; i < lengthof(BLOCKCOLOR); ++i) {
		uint8_t index = 0;
		const color::RGBA rgba = BLOCKCOLOR[i].color;
		if (!palette.tryAdd(rgba, true, &index)) {
			continue;
		}
		for (int j = 0; j < lengthof(BLOCKEMITCOLOR); ++j) {
			if (BLOCKEMITCOLOR[j].blockId != BLOCKCOLOR[i].blockId) {
				continue;
			}
			const color::RGBA emit = BLOCKEMITCOLOR[j].color;
			const float factor = color::getDistance(emit, rgba, color::Distance::HSB);
			palette.setEmit(index, 1.0f - factor);
		}
	}
	return palette.size();
}

bool SMFormat::readSegment(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						   const core::Map<int, int> &blockPal, int headerVersion, int fileVersion,
						   const palette::Palette &palette, int parent, const core::String &name) {
	const int64_t startHeader = stream.pos();
	const bool isSmd2 = fileVersion == 2;
	const int blocks = isSmd2 ? priv::smd2Blocks : priv::smd3Blocks;
	const int segmentTotalSize =
		isSmd2 ? priv::smd2ChunkDataSize : (priv::smd3SegmentDataSize + priv::smd3SegmentHeaderSize);
	Log::debug("read segment (fileVersion=%i, blocks=%i)", fileVersion, blocks);

	if (headerVersion != 0) {
		uint8_t segmentVersion;
		wrap(stream.readUInt8(segmentVersion))
		Log::debug("segmentVersion: %i", (int)segmentVersion);
	}

	uint64_t timestamp;
	wrap(stream.readUInt64BE(timestamp))

	glm::ivec3 segmentPosition;
	wrapBool(readIvec3(stream, segmentPosition))
	Log::debug("segmentPosition: %i:%i:%i", segmentPosition.x, segmentPosition.y, segmentPosition.z);

	bool hasValidData;
	uint32_t compressedSize;
	if (headerVersion == 0) {
		uint8_t segmentType;
		wrap(stream.readUInt8(segmentType))
		int32_t dataLength;
		wrap(stream.readInt32BE(dataLength))
		hasValidData = dataLength > 0;
		compressedSize = dataLength;
	} else { // Valid as of 0.1867, smd file version 1
		hasValidData = stream.readBool();
		wrap(stream.readUInt32BE(compressedSize))
	}
	Log::debug("hasValidData: %i", (int)hasValidData);

	if (!hasValidData) {
		stream.seek(startHeader + segmentTotalSize);
		return true;
	}

	core_assert(stream.pos() - startHeader == (isSmd2 ? priv::smd2SegmentHeaderSize : priv::smd3SegmentHeaderSize));

	io::ZipReadStream blockDataStream(stream, (int)compressedSize);

	const glm::ivec3 origin = segmentPosition - glm::ivec3(16);
	const voxel::Region region(origin, origin + (blocks - 1));
	voxel::RawVolume *volume = new voxel::RawVolume(region);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	node.setPalette(palette);
	bool empty = true;
	int index = -1;
	while (!blockDataStream.eos()) {
		++index;
		uint8_t buf[3];
		// byte orientation : 3
		// byte isActive: 1
		// byte hitpoints: 9
		// ushort blockId: 11
		wrap(blockDataStream.readUInt8(buf[0]))
		wrap(blockDataStream.readUInt8(buf[1]))
		wrap(blockDataStream.readUInt8(buf[2]))
		// SMD2 uses big-endian byte assembly, SMD3 uses little-endian
		const uint32_t blockData =
			isSmd2 ? ((buf[0] << 16) | (buf[1] << 8) | buf[2]) : (buf[0] | (buf[1] << 8) | (buf[2] << 16));
		if (blockData == 0u) {
			continue;
		}
		const uint32_t blockId = core::bits(blockData, 0, 11);
		if (blockId == 0u) {
			continue;
		}
		// const uint32_t hitpoints = core::bits(blockData, 11, 9);
		// const uint32_t active = core::bits(blockData, 20, 1);
		// const uint32_t orientation = core::bits(blockData, 21, 3);
		auto palIter = blockPal.find((int)blockId);
		uint8_t palIndex = 0;
		if (palIter == blockPal.end()) {
			Log::trace("Skip block id %i", (int)blockId);
		} else {
			palIndex = palIter->value;
		}

		glm::ivec3 pos = origin + posByIndex(index, blocks);

		volume->setVoxel(pos, voxel::createVoxel(palette, palIndex));
		empty = false;
	}

	stream.seek(startHeader + segmentTotalSize);
	if (empty) {
		return true;
	}

	node.setName(core::String::format("%s_%i_%i_%i", name.empty() ? "segment" : name.c_str(), segmentPosition.x,
									  segmentPosition.y, segmentPosition.z));
	sceneGraph.emplace(core::move(node), parent);

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
