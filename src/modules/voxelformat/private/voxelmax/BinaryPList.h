/**
 * @file
 * @brief Apple binary property list format
 */

#pragma once

#include "core/Enum.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "io/Stream.h"
#include <stdint.h>

namespace voxelformat {

namespace priv {

enum BPListFormats : uint8_t {
	Null = 0x00,
	False = 0x08,
	True = 0x09,
	Fill = 0x0F, // not used
	Int = 0x10,
	Int8 = 0x11,
	Int16 = 0x12,
	Int32 = 0x14,
	Int64 = 0x18,
	Real = 0x20,
	Real32 = 0x22,
	Real64 = 0x23,
	Date = 0x33,
	Data = 0x40,
	ASCIIString = 0x50,
	Unicode16String = 0x60,
	UID = 0x80,
	Array = 0xA0,
	Set = 0xC0,
	Dict = 0xD0,

	MAX
};

struct BPListTrailer {
	// <= Tiger: "bplist00"
	// == Leopard: "bplist00" and "bplist01"
	// == SnowLeopard: "bplist0?"
	uint8_t unused[5] = {0, 0, 0, 0, 0};
	uint8_t version = 0;
	uint8_t offsetIntSize = 0;
	uint8_t objectRefSize = 0;
	uint64_t numObjects = 0;
	uint64_t topObject = 0;
	uint64_t offsetTableOffset = 0;
	bool valid = false;
};

struct BPListState {
	BPListTrailer trailer;
	int level = 0;
	uint64_t objects = 0;
};

class BinaryPList;

using PListDict = core::StringMap<BinaryPList>;
using PListArray = core::DynamicArray<BinaryPList>;
using PListByteArray = core::DynamicArray<uint8_t>;

union PListData {
	PListDict *_dict;
	PListArray *_array;
	PListByteArray *_data;
	core::String *_string;
	bool _boolean;
	float _float;
	double _double;
	uint8_t _byte;
	uint16_t _short;
	uint32_t _int;
	uint64_t _long;

	constexpr PListData() : _string(nullptr) {
	}
	void copy(BPListFormats type, const PListData &data);
};

class BinaryPList {
private:
	PListData _tagData{};
	BPListFormats _tagType = BPListFormats::MAX;

	static uint64_t readOffset(io::SeekableReadStream &stream, const BPListTrailer &trailer);
	static uint64_t readSizedInt(io::SeekableReadStream &stream, uint8_t numberBytes);
	static bool readObject(io::SeekableReadStream &stream, BPListFormats &type);
	static uint32_t readLength(io::SeekableReadStream &stream, BPListFormats nibble);
	static BinaryPList readNull();
	static BinaryPList readBool(bool value);
	static BinaryPList readArrayAndSet(io::SeekableReadStream &stream, BPListFormats typeLastNibble,
									   BPListState &state);
	static BinaryPList readDict(io::SeekableReadStream &stream, BPListFormats typeLastNibble, BPListState &state);
	static BinaryPList readUID(io::SeekableReadStream &stream, BPListFormats typeLastNibble);
	static BinaryPList readDate(io::SeekableReadStream &stream);
	static BinaryPList readInt(io::SeekableReadStream &stream, BPListFormats typeLastNibble);
	static BinaryPList readReal(io::SeekableReadStream &stream, BPListFormats typeLastNibble);
	static BinaryPList readUTF16Str(io::SeekableReadStream &stream, BPListFormats typeLastNibble);
	static BinaryPList readString(io::SeekableReadStream &stream, BPListFormats typeLastNibble);
	static BinaryPList readData(io::SeekableReadStream &stream, BPListFormats typeLastNibble);

	static bool parseHeader(io::SeekableReadStream &stream);
	static BPListTrailer parseTrailer(io::SeekableReadStream &stream);
	static BinaryPList parse(io::SeekableReadStream &stream, BPListState &state);

public:
	constexpr BinaryPList() {
	}

	constexpr BinaryPList(uint8_t val) : _tagType(BPListFormats::Int8) {
		_tagData._byte = val;
	}

	constexpr BinaryPList(bool val) : _tagType(val ? BPListFormats::True : BPListFormats::False) {
		_tagData._boolean = val;
	}

	constexpr BinaryPList(uint16_t val) : _tagType(BPListFormats::Int16) {
		_tagData._short = val;
	}

	constexpr BinaryPList(uint32_t val) : _tagType(BPListFormats::Int32) {
		_tagData._int = val;
	}

	constexpr BinaryPList(uint64_t val) : _tagType(BPListFormats::Int64) {
		_tagData._long = val;
	}

	constexpr BinaryPList(float val) : _tagType(BPListFormats::Real32) {
		_tagData._float = val;
	}

	constexpr BinaryPList(double val) : _tagType(BPListFormats::Real64) {
		_tagData._double = val;
	}

	constexpr BinaryPList(BPListFormats type, double val) : _tagType((BPListFormats)(type | (1 << 3))) {
		_tagData._double = val;
	}

	constexpr BinaryPList(BPListFormats type, uint8_t val) : _tagType((BPListFormats)(type | (1 << 0))) {
		_tagData._byte = val;
	}

	constexpr BinaryPList(BPListFormats type, uint16_t val) : _tagType((BPListFormats)(type | (1 << 1))) {
		_tagData._short = val;
	}

	constexpr BinaryPList(BPListFormats type, uint32_t val) : _tagType((BPListFormats)(type | (1 << 2))) {
		_tagData._int = val;
	}

	constexpr BinaryPList(BPListFormats type, uint64_t val) : _tagType((BPListFormats)(type | (1 << 3))) {
		_tagData._long = val;
	}

	BinaryPList(core::String &&val);
	BinaryPList(PListDict &&val);
	BinaryPList(PListArray &&val);
	BinaryPList(PListByteArray &&val);
	~BinaryPList();

	BinaryPList(const BinaryPList &val);
	BinaryPList(BinaryPList &&val) noexcept;

	static BinaryPList parse(io::SeekableReadStream &stream);

	bool empty() const;
	size_t size() const;

	bool operator==(const BinaryPList &val) const;

	const BinaryPList &getDictEntry(const core::String &id) const;

	inline bool valid() const {
		return _tagType != BPListFormats::MAX;
	}

	inline bool isDict() const {
		return _tagType == BPListFormats::Dict;
	}

	inline bool isBoolean() const {
		return _tagType == BPListFormats::True || _tagType == BPListFormats::False;
	}

	inline bool isArray() const {
		return _tagType == BPListFormats::Array || _tagType == BPListFormats::Set;
	}

	inline bool isData() const {
		return _tagType == BPListFormats::Data;
	}

	inline bool isInt() const {
		return _tagType & BPListFormats::Int;
	}

	inline bool isReal() const {
		return _tagType & BPListFormats::Real;
	}

	inline bool isDate() const {
		return _tagType & BPListFormats::Date;
	}

	inline bool isUID() const {
		return _tagType == BPListFormats::UID;
	}

	inline bool isNull() const {
		return _tagType == BPListFormats::Null;
	}

	inline bool isString() const {
		return _tagType == BPListFormats::ASCIIString || _tagType == BPListFormats::Unicode16String;
	}

	inline BPListFormats type() const {
		return _tagType;
	}

	uint64_t asInt() const;
	const PListDict &asDict() const;
	bool asBoolean() const;
	const PListArray &asArray() const;
	const PListByteArray &asData() const;
	uint8_t asUInt8() const;
	uint16_t asUInt16() const;
	uint32_t asUInt32() const;
	uint64_t asUInt64() const;
	float asFloat() const;
	double asDouble() const;
	const core::String &asString() const;

	BinaryPList &operator=(const BinaryPList &val);
	BinaryPList &operator=(BinaryPList &&val) noexcept;
};

} // namespace priv

} // namespace voxelformat
