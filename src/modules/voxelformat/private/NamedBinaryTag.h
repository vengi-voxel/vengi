/**
 * @file
 */

#pragma once

#include "core/StandardLib.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "core/concurrent/ThreadPool.h"
#include "io/Stream.h"
#include <stdint.h>

namespace voxel {

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
using NBTCompound = core::StringMap<NamedBinaryTag>;

union TagData {
	NBTCompound *_compound;
	core::DynamicArray<int8_t> *_byteArray;
	core::DynamicArray<int32_t> *_intArray;
	core::DynamicArray<int64_t> *_longArray;
	NBTList *_list;
	core::String *_string;
	float _float;
	double _double;
	int8_t _byte;
	int16_t _short;
	int32_t _int;
	int64_t _long;

	constexpr TagData() : _compound(nullptr) {
	}
	inline explicit operator int8_t() const {
		return _byte;
	}
	inline explicit operator int16_t() const {
		return _short;
	}
	inline explicit operator int32_t() const {
		return _int;
	}
	inline explicit operator int64_t() const {
		return _long;
	}
	inline explicit operator float() const {
		return _float;
	}
	inline explicit operator double() const {
		return _double;
	}
	operator const core::String &() const {
		return *_string;
	}
	operator const NBTCompound &() const {
		return *_compound;
	}
	operator const core::DynamicArray<int8_t> &() const {
		return *_byteArray;
	}
	operator const core::DynamicArray<int32_t> &() const {
		return *_intArray;
	}
	operator const core::DynamicArray<int64_t> &() const {
		return *_longArray;
	}
	operator const NBTList &() const {
		return *_list;
	}
	void copy(TagType type, const TagData &data);
};

struct NamedBinaryTagContext {
	io::ReadStream *stream;
};

/**
 * @note https://minecraft.fandom.com/wiki/NBT_format
 * @note https://wiki.vg/NBT
 */
class NamedBinaryTag {
private:
	TagData _tagData{};
	TagType _tagType = TagType::MAX;

	static NamedBinaryTag parseType(TagType type, NamedBinaryTagContext &ctx, int level);

	static bool readString(io::ReadStream &stream, core::String &str) {
		uint16_t length;
		if (stream.readUInt16BE(length) != 0) {
			return false;
		}
		str.clear();
		for (uint16_t i = 0u; i < length; ++i) {
			uint8_t chr;
			if (stream.readUInt8(chr) != 0) {
				return false;
			}
			str += (char)chr;
		}
		return true;
	}

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

public:
	constexpr NamedBinaryTag() {
	}
	constexpr NamedBinaryTag(bool val);
	constexpr NamedBinaryTag(int64_t val);
	constexpr NamedBinaryTag(int32_t val);
	constexpr NamedBinaryTag(int16_t val);
	constexpr NamedBinaryTag(int8_t val);
	constexpr NamedBinaryTag(double val);
	constexpr NamedBinaryTag(float val);

	NamedBinaryTag(core::String &&val);
	NamedBinaryTag(core::DynamicArray<int64_t> &&val);
	NamedBinaryTag(core::DynamicArray<int32_t> &&val);
	NamedBinaryTag(core::DynamicArray<int8_t> &&val);
	NamedBinaryTag(NBTList &&val);
	NamedBinaryTag(NBTCompound &&val);
	~NamedBinaryTag();

	NamedBinaryTag(const NamedBinaryTag &val);
	NamedBinaryTag(NamedBinaryTag &&val) noexcept;

	static NamedBinaryTag parse(NamedBinaryTagContext &stream);

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
		return _tagData._list;
	}

	inline int64_t int64(int64_t defaultVal = 0) const {
		if (_tagType != TagType::LONG) {
			return defaultVal;
		}
		return _tagData._long;
	}

	inline int32_t int32(int32_t defaultVal = 0) const {
		if (_tagType != TagType::INT) {
			return defaultVal;
		}
		return _tagData._int;
	}

	inline int16_t int16(int16_t defaultVal = 0) const {
		if (_tagType != TagType::SHORT) {
			return defaultVal;
		}
		return _tagData._short;
	}

	inline int8_t int8(int8_t defaultVal = 0) const {
		if (_tagType != TagType::BYTE) {
			return defaultVal;
		}
		return _tagData._byte;
	}

	inline const core::String* string() const {
		if (_tagType != TagType::STRING) {
			return nullptr;
		}
		return _tagData._string;
	}

	inline const core::DynamicArray<int8_t> *byteArray() const {
		if (_tagType != TagType::BYTE_ARRAY) {
			return nullptr;
		}
		return _tagData._byteArray;
	}

	inline const core::DynamicArray<int32_t> *intArray() const {
		if (_tagType != TagType::INT_ARRAY) {
			return nullptr;
		}
		return _tagData._intArray;
	}

	inline const core::DynamicArray<int64_t> *longArray() const {
		if (_tagType != TagType::LONG_ARRAY) {
			return nullptr;
		}
		return _tagData._longArray;
	}

	inline const NBTCompound* compound() const {
		if (_tagType != TagType::COMPOUND) {
			return nullptr;
		}
		return _tagData._compound;
	}

	const NamedBinaryTag &get(const core::String &name) const {
		static const NamedBinaryTag INVALID;
		if (_tagType != TagType::COMPOUND) {
			return INVALID;
		}
		auto iter = _tagData._compound->find(name);
		if (iter == _tagData._compound->end()) {
			return INVALID;
		}
		return iter->second;
	}

	NamedBinaryTag &operator=(const NamedBinaryTag &val);
	NamedBinaryTag &operator=(NamedBinaryTag &&val) noexcept;
};

} // namespace priv
} // namespace voxel
