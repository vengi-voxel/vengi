/**
 * @file
 */

#include "NamedBinaryTag.h"
#include "core/Log.h"
#include "io/MemoryReadStream.h"

namespace voxelformat {

namespace priv {

constexpr NamedBinaryTag::NamedBinaryTag(int8_t val) : _tagType(TagType::BYTE) {
	_tagData._byte = val;
}

constexpr NamedBinaryTag::NamedBinaryTag(int16_t val) : _tagType(TagType::SHORT) {
	_tagData._short = val;
}

constexpr NamedBinaryTag::NamedBinaryTag(int32_t val) : _tagType(TagType::INT) {
	_tagData._int = val;
}

constexpr NamedBinaryTag::NamedBinaryTag(int64_t val) : _tagType(TagType::LONG) {
	_tagData._long = val;
}

constexpr NamedBinaryTag::NamedBinaryTag(float val) : _tagType(TagType::FLOAT) {
	_tagData._float = val;
}

constexpr NamedBinaryTag::NamedBinaryTag(double val) : _tagType(TagType::DOUBLE) {
	_tagData._double = val;
}

NamedBinaryTag::NamedBinaryTag(core::String &&val) : _tagType(TagType::STRING) {
	_tagData._string = new core::String(core::move(val));
}

NamedBinaryTag::NamedBinaryTag(core::DynamicArray<int8_t> &&val) : _tagType(TagType::BYTE_ARRAY) {
	_tagData._byteArray = new core::DynamicArray<int8_t>(core::move(val));
}

NamedBinaryTag::NamedBinaryTag(core::DynamicArray<int32_t> &&val) : _tagType(TagType::INT_ARRAY) {
	_tagData._intArray = new core::DynamicArray<int32_t>(core::move(val));
}

NamedBinaryTag::NamedBinaryTag(core::DynamicArray<int64_t> &&val) : _tagType(TagType::LONG_ARRAY) {
	_tagData._longArray = new core::DynamicArray<int64_t>(core::move(val));
}

NamedBinaryTag::NamedBinaryTag(NBTList &&val) : _tagType(TagType::LIST) {
	_tagData._list = new NBTList(core::move(val));
}

NamedBinaryTag::NamedBinaryTag(NBTCompound &&val) : _tagType(TagType::COMPOUND) {
	_tagData._compound = new NBTCompound(core::move(val));
}

NamedBinaryTag::NamedBinaryTag(const NamedBinaryTag &val) {
	_tagType = val._tagType;
	_tagData.copy(val._tagType, val._tagData);
}

void TagData::copy(TagType type, const TagData &data) {
	core_assert(_compound == nullptr);
	switch (type) {
	case TagType::COMPOUND:
		_compound = new NBTCompound(*data._compound);
		break;
	case TagType::BYTE_ARRAY:
		_byteArray = new core::DynamicArray<int8_t>(*data._byteArray);
		break;
	case TagType::INT_ARRAY:
		_intArray = new core::DynamicArray<int32_t>(*data._intArray);
		break;
	case TagType::LONG_ARRAY:
		_longArray = new core::DynamicArray<int64_t>(*data._longArray);
		break;
	case TagType::LIST:
		_list = new NBTList(*data._list);
		break;
	case TagType::STRING:
		_string = new core::String(*data._string);
		break;
	default:
		*this = data;
		break;
	}
}

NamedBinaryTag::NamedBinaryTag(NamedBinaryTag &&val) noexcept {
	_tagType = val._tagType;
	_tagData = val._tagData;

	val._tagType = TagType::MAX;
	val._tagData._compound = nullptr;
}

NamedBinaryTag::~NamedBinaryTag() {
	switch (_tagType) {
	case TagType::COMPOUND:
		delete _tagData._compound;
		break;
	case TagType::BYTE_ARRAY:
		delete _tagData._byteArray;
		break;
	case TagType::INT_ARRAY:
		delete _tagData._intArray;
		break;
	case TagType::LONG_ARRAY:
		delete _tagData._longArray;
		break;
	case TagType::LIST:
		delete _tagData._list;
		break;
	case TagType::STRING:
		delete _tagData._string;
		break;
	case TagType::MAX:
		break;
	default:
		core_assert(isPrimitiveType(_tagType));
		break;
	}
	_tagData._compound = nullptr;
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

	if (name == nullptr || *name == '\0') {
		stream.writeStringFormat(false, "%*s%s", level, " ", Names[(int)tag.type()]);
	} else {
		stream.writeStringFormat(false, "%*s%s[%s]", level, " ", name, Names[(int)tag.type()]);
	}
	switch (tag.type()) {
	case TagType::BYTE:
		stream.writeStringFormat(false, " = %i", tag._tagData._byte);
		break;
	case TagType::SHORT:
		stream.writeStringFormat(false, " = %i", tag._tagData._short);
		break;
	case TagType::FLOAT:
		stream.writeStringFormat(false, " = %f", tag._tagData._float);
		break;
	case TagType::DOUBLE:
		stream.writeStringFormat(false, " = %f", tag._tagData._double);
		break;
	case TagType::INT:
		stream.writeStringFormat(false, " = %i", tag._tagData._int);
		break;
	case TagType::LONG:
		stream.writeStringFormat(false, " = %li", (long int)tag._tagData._long);
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

NamedBinaryTag NamedBinaryTag::parse(NamedBinaryTagContext &ctx) {
	TagType type;
	if (!readType(*ctx.stream, type) || type != TagType::COMPOUND) {
		return NamedBinaryTag{};
	}
	core::String rootName;
	if (!readString(*ctx.stream, rootName)) {
		return NamedBinaryTag{};
	}
	return parseType(type, ctx, 0);
}

NamedBinaryTag NamedBinaryTag::parseType(TagType type, NamedBinaryTagContext &ctx, int level) {
	switch (type) {
	case TagType::COMPOUND: {
		NBTCompound compound;
		TagType type;
		while (readType(*ctx.stream, type) && type != TagType::END) {
			core::String name;
			if (!readString(*ctx.stream, name)) {
				return NamedBinaryTag{};
			}
			Log::trace("%*sFound %s of type %i", level * 3, " ", name.c_str(), (int)type);
			compound.emplace(name, parseType(type, ctx, level + 1));
		}
		return NamedBinaryTag{core::move(compound)};
	}
	case TagType::BYTE: {
		int8_t val;
		if (ctx.stream->readInt8(val) != 0) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::SHORT: {
		int16_t val;
		if (ctx.stream->readInt16BE(val) != 0) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::FLOAT: {
		float val;
		if (ctx.stream->readFloatBE(val) != 0) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::DOUBLE: {
		union {
			double d;
			uint64_t l;
		} u;
		if (ctx.stream->readUInt64BE(u.l) != 0) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{u.d};
	}
	case TagType::INT: {
		int32_t val;
		if (ctx.stream->readInt32BE(val) != 0) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::LONG: {
		int64_t val;
		if (ctx.stream->readInt64BE(val) != 0) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{val};
	}
	case TagType::BYTE_ARRAY: {
		uint32_t length;
		if (ctx.stream->readUInt32BE(length) != 0) {
			return NamedBinaryTag{};
		}
		core::DynamicArray<int8_t> array;
		array.reserve(length);
		for (size_t i = 0; i < length; i++) {
			int8_t val;
			if (ctx.stream->readInt8(val) != 0) {
				return NamedBinaryTag{};
			}
			array.push_back(val);
		}
		return NamedBinaryTag{core::move(array)};
	}
	case TagType::INT_ARRAY: {
		uint32_t length;
		if (ctx.stream->readUInt32BE(length) != 0) {
			return NamedBinaryTag{};
		}
		core::DynamicArray<int32_t> array;
		array.reserve(length);
		for (size_t i = 0; i < length; i++) {
			int32_t val;
			if (ctx.stream->readInt32BE(val) != 0) {
				return NamedBinaryTag{};
			}
			array.push_back(val);
		}
		return NamedBinaryTag{core::move(array)};
	}
	case TagType::LONG_ARRAY: {
		uint32_t length;
		if (ctx.stream->readUInt32BE(length) != 0) {
			return NamedBinaryTag{};
		}
		core::DynamicArray<int64_t> array;
		array.reserve(length);
		for (size_t i = 0; i < length; i++) {
			int64_t val;
			if (ctx.stream->readInt64BE(val) != 0) {
				return NamedBinaryTag{};
			}
			array.push_back(val);
		}
		return NamedBinaryTag{core::move(array)};
	}
	case TagType::LIST: {
		TagType contentType;
		if (!readType(*ctx.stream, contentType)) {
			return NamedBinaryTag{};
		}
		uint32_t length;
		if (ctx.stream->readUInt32BE(length) != 0) {
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
		if (!readString(*ctx.stream, str)) {
			return NamedBinaryTag{};
		}
		return NamedBinaryTag{core::move(str)};
	}
	default:
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
			*_tagData._string = *val._tagData._string;
			break;
		case TagType::BYTE_ARRAY:
			*_tagData._byteArray = *val._tagData._byteArray;
			break;
		case TagType::INT_ARRAY:
			*_tagData._intArray = *val._tagData._intArray;
			break;
		case TagType::LONG_ARRAY:
			*_tagData._longArray = *val._tagData._longArray;
			break;
		case TagType::LIST:
			*_tagData._list = *val._tagData._list;
			break;
		case TagType::COMPOUND:
			*_tagData._compound = *val._tagData._compound;
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
