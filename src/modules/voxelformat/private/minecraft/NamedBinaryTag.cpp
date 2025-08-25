/**
 * @file
 */

#include "NamedBinaryTag.h"
#include "core/Log.h"
#include "core/ArrayLength.h"
#include "io/BufferedReadWriteStream.h"

namespace voxelformat {

namespace priv {

bool NamedBinaryTagContext::readString(core::String &str) {
	if (bedrock) {
		return stream->readPascalStringUInt16LE(str);
	}
	return stream->readPascalStringUInt16BE(str);
}

bool NamedBinaryTagContext::readUInt32(uint32_t &val) {
	if (bedrock) {
		return stream->readUInt32(val);
	}
	return stream->readUInt32BE(val);
}

bool NamedBinaryTagContext::readInt32(int32_t &val) {
	if (bedrock) {
		return stream->readInt32(val);
	}
	return stream->readInt32BE(val);
}

bool NamedBinaryTagContext::readInt16(int16_t &val) {
	if (bedrock) {
		return stream->readInt16(val);
	}
	return stream->readInt16BE(val);
}

bool NamedBinaryTagContext::readFloat(float &val) {
	if (bedrock) {
		return stream->readFloat(val);
	}
	return stream->readFloatBE(val);
}

bool NamedBinaryTagContext::readUInt64(uint64_t &val) {
	if (bedrock) {
		return stream->readUInt64(val);
	}
	return stream->readUInt64BE(val);
}

bool NamedBinaryTagContext::readInt64(int64_t &val) {
	if (bedrock) {
		return stream->readInt64(val);
	}
	return stream->readInt64BE(val);
}

NamedBinaryTag::NamedBinaryTag(core::String &&val) : _tagType(TagType::STRING) {
	_tagData._string = core::move(val);
}

NamedBinaryTag::NamedBinaryTag(const core::String &val) : _tagType(TagType::STRING) {
	_tagData._string = val;
}

NamedBinaryTag::NamedBinaryTag(core::Buffer<int8_t> &&val) : _tagType(TagType::BYTE_ARRAY) {
	_tagData._byteArray = core::move(val);
}

NamedBinaryTag::NamedBinaryTag(core::Buffer<uint8_t> &&val) : _tagType(TagType::BYTE_ARRAY) {
	_tagData._byteArray.append(val);
}

NamedBinaryTag::NamedBinaryTag(core::Buffer<int32_t> &&val) : _tagType(TagType::INT_ARRAY) {
	_tagData._intArray = core::move(val);
}

NamedBinaryTag::NamedBinaryTag(core::Buffer<int64_t> &&val) : _tagType(TagType::LONG_ARRAY) {
	_tagData._longArray = core::move(val);
}

NamedBinaryTag::NamedBinaryTag(NBTList &&val) : _tagType(TagType::LIST) {
	_tagData._list = core::move(val);
}

NamedBinaryTag::NamedBinaryTag(NBTCompound &&val) : _tagType(TagType::COMPOUND) {
	_tagData._compound = core::move(val);
}

NamedBinaryTag::NamedBinaryTag(const NamedBinaryTag &val) {
	_tagType = val._tagType;
	_tagData.copy(val._tagType, val._tagData);
}

void TagData::copy(TagType type, const TagData &data) {
	switch (type) {
	case TagType::COMPOUND:
		_compound = data._compound;
		break;
	case TagType::BYTE_ARRAY:
		_byteArray = data._byteArray;
		break;
	case TagType::INT_ARRAY:
		_intArray = data._intArray;
		break;
	case TagType::LONG_ARRAY:
		_longArray = data._longArray;
		break;
	case TagType::LIST:
		_list = data._list;
		break;
	case TagType::STRING:
		_string = data._string;
		break;
	default:
		*this = data;
		break;
	}
}

NamedBinaryTag::NamedBinaryTag(NamedBinaryTag &&other) noexcept : _tagData(core::move(other._tagData)), _tagType(other._tagType) {
	other._tagType = TagType::MAX;
}

NamedBinaryTag::~NamedBinaryTag() {
}

void NamedBinaryTag::dump_r(io::WriteStream &stream, const char *name, const NamedBinaryTag &tag, int level) {
	static const char *Names[] {
		"END",
		"BYTE",
		"SHORT",
		"INT",
		"LONG",
		"FLOAT",
		"DOUBLE",
		"BYTE_ARRAY",
		"STRING",
		"LIST",
		"COMPOUND",
		"INT_ARRAY",
		"LONG_ARRAY"
	};
	static_assert((int)TagType::MAX == lengthof(Names), "Array size doesn't match tag types");

	int type = (int)tag.type();
	if (type < 0 || type >= lengthof(Names)) {
		Log::error("Invalid tag type %i", type);
		return;
	}

	if (name == nullptr || *name == '\0') {
		stream.writeStringFormat(false, "%*s%s", level, " ", Names[type]);
	} else {
		stream.writeStringFormat(false, "%*s%s[%s]", level, " ", name, Names[type]);
	}
	switch (tag.type()) {
	case TagType::BYTE:
		stream.writeStringFormat(false, " = %i", tag._tagData._val._byte);
		break;
	case TagType::SHORT:
		stream.writeStringFormat(false, " = %i", tag._tagData._val._short);
		break;
	case TagType::FLOAT:
		stream.writeStringFormat(false, " = %f", tag._tagData._val._float);
		break;
	case TagType::DOUBLE:
		stream.writeStringFormat(false, " = %f", tag._tagData._val._double);
		break;
	case TagType::INT:
		stream.writeStringFormat(false, " = %i", tag._tagData._val._int);
		break;
	case TagType::LONG:
		stream.writeStringFormat(false, " = %li", (long int)tag._tagData._val._long);
		break;
	case TagType::STRING:
		stream.writeStringFormat(false, " = %s", tag.string()->c_str());
		break;
	case TagType::COMPOUND:
		stream.writeStringFormat(false, " (%i)\n", (int)(*tag.compound()).size());
		for (const auto &c : *tag.compound()) {
			dump_r(stream, c->first.c_str(), c->second, level + 1);
		}
		break;
	case TagType::LIST:
		stream.writeStringFormat(false, " (%i)\n", (int)(*tag.list()).size());
		for (const auto& c : *tag.list()) {
			dump_r(stream, "", c, level + 1);
		}
		break;
	case TagType::BYTE_ARRAY:
	case TagType::INT_ARRAY:
	case TagType::LONG_ARRAY:
	case TagType::END:
	case TagType::MAX:
		break;
	}
	stream.writeString("\n", false);
}

void NamedBinaryTag::dump(io::WriteStream &stream) const {
	dump_r(stream, "", *this, 0);
	stream.writeUInt8(0);
}

void NamedBinaryTag::print() const {
	io::BufferedReadWriteStream stream;
	dump(stream);
	stream.seek(0);
	char buf[16000];
	bool ret = stream.readString(lengthof(buf), buf);
	Log::error("%s", buf);
	while (ret) {
		ret = stream.readString(lengthof(buf), buf);
		Log::error("%s", buf);
	}
}

bool NamedBinaryTag::write(const NamedBinaryTag &tag, const core::String &rootTagName, io::WriteStream &stream) {
	if (!writeTagType(stream, tag.type())) {
		return false;
	}
	if (!stream.writePascalStringUInt16BE(rootTagName)) {
		return false;
	}
	return writeType(stream, tag);
}

bool NamedBinaryTag::writeTagType(io::WriteStream &stream, TagType type) {
	return stream.writeUInt8((uint8_t)type);
}

bool NamedBinaryTag::writeType(io::WriteStream &stream, const NamedBinaryTag &tag) {
	switch (tag.type()) {
	case TagType::COMPOUND: {
		const NBTCompound* compound = tag.compound();
		for (const auto &c : *compound) {
			if (!writeTagType(stream, c->second.type())) {
				return false;
			}
			if (!stream.writePascalStringUInt16BE(c->first)) {
				return false;
			}
			if (!writeType(stream, c->second)) {
				return false;
			}
		}
		if (!writeTagType(stream, TagType::END)) {
			return false;
		}
		return true;
	}
	case TagType::BYTE:
		return stream.writeInt8(tag.int8());
	case TagType::SHORT:
		return stream.writeInt16BE(tag.int16());
	case TagType::FLOAT:
		return stream.writeFloatBE(tag.float32());
	case TagType::DOUBLE: {
		union {
			double d;
			uint64_t l;
		} u;
		u.d = tag.float64();
		return stream.writeUInt64BE(u.l);
	}
	case TagType::INT:
		return stream.writeInt32BE(tag.int32());
	case TagType::LONG:
		return stream.writeInt64BE(tag.int64());
	case TagType::BYTE_ARRAY: {
		const size_t length = tag.byteArray()->size();
		if (!stream.writeUInt32BE(length)) {
			return false;
		}
		for (size_t i = 0; i < length; i++) {
			if (!stream.writeInt8((*tag.byteArray())[i])) {
				return false;
			}
		}
		return true;
	}
	case TagType::INT_ARRAY: {
		const size_t length = tag.intArray()->size();
		if (!stream.writeUInt32BE(length)) {
			return false;
		}
		for (size_t i = 0; i < length; i++) {
			if (!stream.writeInt32((*tag.intArray())[i])) {
				return false;
			}
		}
		return true;
	}
	case TagType::LONG_ARRAY: {
		const size_t length = tag.longArray()->size();
		if (!stream.writeUInt32BE(length)) {
			return false;
		}
		for (size_t i = 0; i < length; i++) {
			if (!stream.writeInt64((*tag.longArray())[i])) {
				return false;
			}
		}
		return true;
	}
	case TagType::LIST: {
		if (tag.list()->empty()) {
			return writeTagType(stream, TagType::END);
		}
		if (!writeTagType(stream, tag.list()->front().type())) {
			return false;
		}

		const size_t length = tag.list()->size();
		if (!stream.writeUInt32BE(length)) {
			return false;
		}
		for (size_t i = 0; i < length; i++) {
			if (!writeType(stream, (*tag.list())[i])) {
				return false;
			}
		}
		return true;
	}
	case TagType::STRING:
		return stream.writePascalStringUInt16BE(*tag.string());
	default:
		return false;
	}
}

NamedBinaryTag NamedBinaryTag::parse(NamedBinaryTagContext &ctx) {
	TagType type = TagType::MAX;
	if (!readType(*ctx.stream, type)) {
		Log::debug("Failed to read type: %i", (int)type);
		return NamedBinaryTag{};
	}
	if (type != TagType::COMPOUND) {
		// TODO: VOXELFORMAT: in bedrock this is sometimes a LIST
		Log::debug("Root tag is not a compound but %i", (int)type);
		return NamedBinaryTag{};
	}
	core::String rootName;
	if (!ctx.readString(rootName)) {
		Log::debug("Failed to read root name");
		return NamedBinaryTag{};
	}
	return parseType(type, ctx, 0);
}

NamedBinaryTag NamedBinaryTag::parseType(TagType type, NamedBinaryTagContext &ctx, int level) {
	switch (type) {
	case TagType::COMPOUND: {
		NBTCompound compound;
		TagType subType;
		while (readType(*ctx.stream, subType) && subType != TagType::END) {
			core::String name;
			if (!ctx.readString(name)) {
				Log::debug("Failed to read compound name");
				return NamedBinaryTag{};
			}
			Log::trace("%*sFound %s of type %i", level * 3, " ", name.c_str(), (int)subType);
			compound.emplace(name, parseType(subType, ctx, level + 1));
		}
		return NamedBinaryTag{core::move(compound)};
	}
	case TagType::BYTE: {
		int8_t val;
		if (ctx.stream->readInt8(val) != 0) {
			Log::debug("Failed to read byte");
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::SHORT: {
		int16_t val;
		if (ctx.readInt16(val) != 0) {
			Log::debug("Failed to read short");
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::FLOAT: {
		float val;
		if (ctx.readFloat(val) != 0) {
			Log::debug("Failed to read float");
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::DOUBLE: {
		union {
			double d;
			uint64_t l;
		} u;
		if (ctx.readUInt64(u.l) != 0) {
			Log::debug("Failed to read double");
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{u.d};
	}
	case TagType::INT: {
		int32_t val;
		if (ctx.readInt32(val) != 0) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::LONG: {
		int64_t val;
		if (ctx.readInt64(val) != 0) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::BYTE_ARRAY: {
		uint32_t length;
		if (ctx.readUInt32(length) != 0) {
			return NamedBinaryTag{};
		}
		core::Buffer<int8_t> array;
		array.reserve(length);
		bool error = false;
		array.append(length, [&ctx, &error] (int i) {
			int8_t val;
			if (ctx.stream->readInt8(val) != 0) {
				error = true;
			}
			return val;
		});
		if (error) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{core::move(array)};
	}
	case TagType::INT_ARRAY: {
		uint32_t length;
		if (ctx.readUInt32(length) != 0) {
			Log::debug("Failed to read int array length");
			return NamedBinaryTag{};
		}
		core::Buffer<int32_t> array;
		array.reserve(length);
		bool error = false;
		array.append(length, [&ctx, &error] (int i) {
			int32_t val;
			if (ctx.readInt32(val) != 0) {
				error = true;
			}
			return val;
		});
		return NamedBinaryTag{core::move(array)};
	}
	case TagType::LONG_ARRAY: {
		uint32_t length;
		if (ctx.readUInt32(length) != 0) {
			Log::debug("Failed to read long array length");
			return NamedBinaryTag{};
		}
		core::Buffer<int64_t> array;
		array.reserve(length);
		bool error = false;
		array.append(length, [&ctx, &error] (int i) {
			int64_t val;
			if (ctx.readInt64(val) != 0) {
				error = true;
			}
			return val;
		});
		return NamedBinaryTag{core::move(array)};
	}
	case TagType::LIST: {
		TagType contentType;
		if (!readType(*ctx.stream, contentType)) {
			Log::debug("Failed to read list content type");
			return NamedBinaryTag{};
		}
		uint32_t length;
		if (ctx.readUInt32(length) != 0) {
			Log::debug("Failed to read list length");
			return NamedBinaryTag{};
		}
		NBTList list;
		list.reserve(length);

		if (contentType > TagType::END && contentType < TagType::MAX) {
			for (uint32_t i = 0; i < length; i++) {
				list.emplace_back(parseType(contentType, ctx, level + 1));
			}
		}
		return NamedBinaryTag{core::move(list)};
	}
	case TagType::STRING: {
		core::String str;
		if (!ctx.readString(str)) {
			Log::debug("Failed to read string");
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{core::move(str)};
	}
	default:
		Log::debug("Unknown tag type %i", (int)type);
		return NamedBinaryTag{};
	}
}

NamedBinaryTag &NamedBinaryTag::operator=(const NamedBinaryTag &val) {
	if (_tagType == TagType::MAX) {
		_tagType = val._tagType;
		_tagData.copy(val._tagType, val._tagData);
	} else if (_tagType == val._tagType) {
		switch (_tagType) {
		case TagType::STRING:
			_tagData._string = val._tagData._string;
			break;
		case TagType::BYTE_ARRAY:
			_tagData._byteArray = val._tagData._byteArray;
			break;
		case TagType::INT_ARRAY:
			_tagData._intArray = val._tagData._intArray;
			break;
		case TagType::LONG_ARRAY:
			_tagData._longArray = val._tagData._longArray;
			break;
		case TagType::LIST:
			_tagData._list = val._tagData._list;
			break;
		case TagType::COMPOUND:
			_tagData._compound = val._tagData._compound;
			break;
		default:
			core_assert(isPrimitiveType(_tagType));
			_tagData = val._tagData;
			break;
		}
	}
	return *this;
}

NamedBinaryTag &NamedBinaryTag::operator=(NamedBinaryTag &&val) noexcept {
	if (this == &val) {
		return *this;
	}
	this->~NamedBinaryTag();

	_tagData = val._tagData;
	_tagType = val._tagType;

	val._tagType = TagType::MAX;
	val._tagData._string = nullptr;
	return *this;
}

} // namespace priv
} // namespace voxel
