/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicMap.h"
#include "io/Stream.h"
#include <stdint.h>

namespace voxelformat {

namespace priv {

class NamedBinaryTag;

enum class TagType : uint8_t {
	END = 0,
	BYTE = 1,
	SHORT = 2,
	INT = 3,
	LONG = 4,
	FLOAT = 5,
	DOUBLE = 6,
	BYTE_ARRAY = 7,
	STRING = 8,
	LIST = 9,
	COMPOUND = 10,
	INT_ARRAY = 11,
	LONG_ARRAY = 12,

	MAX
};

using NBTList = core::DynamicArray<NamedBinaryTag>;
using NBTCompound = core::DynamicMap<core::String, NamedBinaryTag, 7, core::StringHash, core::privdynamicmap::EqualCompare, 4>;

struct TagData {
	NBTCompound _compound;
	core::Buffer<int8_t> _byteArray;
	core::Buffer<int32_t> _intArray;
	core::Buffer<int64_t> _longArray;
	NBTList _list;
	core::String _string;
	union val {
		float _float;
		double _double;
		int8_t _byte;
		int16_t _short;
		int32_t _int;
		int64_t _long;
	} _val;

	TagData() {
	}
	inline explicit operator int8_t() const {
		return _val._byte;
	}
	inline explicit operator int16_t() const {
		return _val._short;
	}
	inline explicit operator int32_t() const {
		return _val._int;
	}
	inline explicit operator int64_t() const {
		return _val._long;
	}
	inline explicit operator float() const {
		return _val._float;
	}
	inline explicit operator double() const {
		return _val._double;
	}
	operator const core::String &() const {
		return _string;
	}
	operator const NBTCompound &() const {
		return _compound;
	}
	operator const core::Buffer<int8_t> &() const {
		return _byteArray;
	}
	operator const core::Buffer<int32_t> &() const {
		return _intArray;
	}
	operator const core::Buffer<int64_t> &() const {
		return _longArray;
	}
	operator const NBTList &() const {
		return _list;
	}
	void copy(TagType type, const TagData &data);
};

struct NamedBinaryTagContext {
	io::ReadStream *stream = nullptr;
	bool bedrock = false;

	bool readString(core::String &out);
	bool readUInt32(uint32_t &val);
	bool readInt32(int32_t &val);
	bool readUInt64(uint64_t &val);
	bool readInt64(int64_t &val);
	bool readInt16(int16_t &val);
	bool readFloat(float &val);
};

/**
 * @note https://minecraft.wiki/w/NBT_format
 * @note https://wiki.vg/NBT
 */
class NamedBinaryTag {
private:
	TagData _tagData{};
	TagType _tagType = TagType::MAX;

	static bool writeTagType(io::WriteStream &stream, TagType type);
	static bool writeType(io::WriteStream &stream, const NamedBinaryTag &tag);

	static NamedBinaryTag parseType(TagType type, NamedBinaryTagContext &ctx, int level);

	static bool readType(io::ReadStream &stream, TagType &type) {
		return stream.read(&type, sizeof(type)) == sizeof(type);
	}

	static constexpr bool isPointerType(TagType type) {
		return type == TagType::BYTE_ARRAY || type == TagType::INT_ARRAY || type == TagType::LONG_ARRAY ||
			   type == TagType::STRING || type == TagType::LIST || type == TagType::COMPOUND;
	}

	static constexpr bool isPrimitiveType(TagType type) {
		return !isPointerType(type);
	}

	static void dump_r(io::WriteStream &stream, const char *name, const NamedBinaryTag &tag, int level);

public:
	NamedBinaryTag() {
	}

	NamedBinaryTag(int8_t val) : _tagType(TagType::BYTE) {
		_tagData._val._byte = val;
	}

	NamedBinaryTag(int16_t val) : _tagType(TagType::SHORT) {
		_tagData._val._short = val;
	}

	NamedBinaryTag(int32_t val) : _tagType(TagType::INT) {
		_tagData._val._int = val;
	}

	NamedBinaryTag(int64_t val) : _tagType(TagType::LONG) {
		_tagData._val._long = val;
	}

	NamedBinaryTag(float val) : _tagType(TagType::FLOAT) {
		_tagData._val._float = val;
	}

	NamedBinaryTag(double val) : _tagType(TagType::DOUBLE) {
		_tagData._val._double = val;
	}

	NamedBinaryTag(core::String &&val);
	NamedBinaryTag(const core::String &val);
	NamedBinaryTag(core::Buffer<int64_t> &&val);
	NamedBinaryTag(core::Buffer<int32_t> &&val);
	NamedBinaryTag(core::Buffer<int8_t> &&val);
	NamedBinaryTag(core::Buffer<uint8_t> &&val);
	NamedBinaryTag(NBTList &&val);
	NamedBinaryTag(NBTCompound &&val);
	~NamedBinaryTag();

	NamedBinaryTag(const NamedBinaryTag &val);
	NamedBinaryTag(NamedBinaryTag &&val) noexcept;

	static NamedBinaryTag parse(NamedBinaryTagContext &ctx);
	static bool write(const NamedBinaryTag &tag, const core::String &rootTagName, io::WriteStream &ctx);

	void dump(io::WriteStream &stream) const;
	void print() const;

	inline bool valid() const {
		return _tagType != TagType::MAX;
	}

	inline TagType type() const {
		return _tagType;
	}

	inline const TagData& data() const {
		return _tagData;
	}

	inline const NBTList* list() const {
		if (_tagType != TagType::LIST) {
			return nullptr;
		}
		return &_tagData._list;
	}

	inline int64_t int64(int64_t defaultVal = 0) const {
		if (_tagType != TagType::LONG) {
			return defaultVal;
		}
		return _tagData._val._long;
	}

	inline int32_t int32(int32_t defaultVal = 0) const {
		if (_tagType != TagType::INT) {
			return defaultVal;
		}
		return _tagData._val._int;
	}

	inline int16_t int16(int16_t defaultVal = 0) const {
		if (_tagType != TagType::SHORT) {
			return defaultVal;
		}
		return _tagData._val._short;
	}

	inline float float32(float defaultVal = 0.0f) const {
		if (_tagType != TagType::FLOAT) {
			return defaultVal;
		}
		return _tagData._val._float;
	}

	inline double float64(double defaultVal = 0.0f) const {
		if (_tagType != TagType::DOUBLE) {
			return defaultVal;
		}
		return _tagData._val._double;
	}

	inline int8_t int8(int8_t defaultVal = 0) const {
		if (_tagType != TagType::BYTE) {
			return defaultVal;
		}
		return _tagData._val._byte;
	}

	inline const core::String* string() const {
		if (_tagType != TagType::STRING) {
			return nullptr;
		}
		return &_tagData._string;
	}

	inline const core::Buffer<int8_t> *byteArray() const {
		if (_tagType != TagType::BYTE_ARRAY) {
			return nullptr;
		}
		return &_tagData._byteArray;
	}

	inline const core::Buffer<int32_t> *intArray() const {
		if (_tagType != TagType::INT_ARRAY) {
			return nullptr;
		}
		return &_tagData._intArray;
	}

	inline const core::Buffer<int64_t> *longArray() const {
		if (_tagType != TagType::LONG_ARRAY) {
			return nullptr;
		}
		return &_tagData._longArray;
	}

	inline const NBTCompound* compound() const {
		if (_tagType != TagType::COMPOUND) {
			return nullptr;
		}
		return &_tagData._compound;
	}

	const NamedBinaryTag &get(const core::String &name) const {
		static const NamedBinaryTag INVALID;
		if (_tagType != TagType::COMPOUND) {
			return INVALID;
		}
		auto iter = _tagData._compound.find(name);
		if (iter == _tagData._compound.end()) {
			return INVALID;
		}
		return iter->second;
	}

	NamedBinaryTag &operator=(const NamedBinaryTag &val);
	NamedBinaryTag &operator=(NamedBinaryTag &&val) noexcept;
};

} // namespace priv
} // namespace voxel
