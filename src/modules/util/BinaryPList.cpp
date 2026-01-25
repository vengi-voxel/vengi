
/**
 * @file
 */

#include "BinaryPList.h"
#include "core/Log.h"
#include "core/collection/DynamicArray.h"
#include "io/Stream.h"
#include <stdint.h>

namespace util {

void PListData::copy(BPListFormats type, const PListData &data) {
	BPListFormats typeFirstNibble = (BPListFormats)(type & 0xF0);
	switch (typeFirstNibble) {
	case BPListFormats::ASCIIString:
	case BPListFormats::Unicode16String:
		_string = new core::String(*data._string);
		break;
	case BPListFormats::Dict:
		_dict = new PListDict(*data._dict);
		break;
	case BPListFormats::Array:
	case BPListFormats::Set:
		_array = new PListArray(*data._array);
		break;
	case BPListFormats::Data:
		_data = new PListByteArray(*data._data);
		break;
	default:
		*this = data;
		break;
	}
}

bool BinaryPList::operator==(const BinaryPList &val) const {
	return this == &val;
}

const PListDict &BinaryPList::asDict() const {
	core_assert(isDict());
	return *_tagData._dict;
}

bool BinaryPList::asBoolean() const {
	core_assert(isBoolean());
	return _tagData._boolean;
}

const PListArray &BinaryPList::asArray() const {
	core_assert(isArray());
	return *_tagData._array;
}

const PListByteArray &BinaryPList::asData() const {
	core_assert(isData());
	return *_tagData._data;
}

uint64_t BinaryPList::asInt() const {
	core_assert(isInt());
	if (_tagType == BPListFormats::Int8) {
		return _tagData._byte;
	}
	if (_tagType == BPListFormats::Int16) {
		return _tagData._short;
	}
	if (_tagType == BPListFormats::Int32) {
		return _tagData._int;
	}
	return _tagData._long;
}

uint8_t BinaryPList::asUInt8() const {
	core_assert_msg(_tagType == BPListFormats::Int8, "Expected Int8, got %i", _tagType);
	return _tagData._byte;
}

uint16_t BinaryPList::asUInt16() const {
	core_assert_msg(_tagType == BPListFormats::Int16, "Expected Int16, got %i", _tagType);
	return _tagData._short;
}

uint32_t BinaryPList::asUInt32() const {
	core_assert_msg(_tagType == BPListFormats::Int32, "Expected Int32, got %i", _tagType);
	return _tagData._int;
}

uint64_t BinaryPList::asUInt64() const {
	core_assert_msg(_tagType == BPListFormats::Int64, "Expected Int64, got %i", _tagType);
	return _tagData._long;
}

double BinaryPList::asReal() const {
	if (_tagType == BPListFormats::Real32) {
		return asFloat();
	}
	return asDouble();
}

float BinaryPList::asFloat() const {
	core_assert_msg(_tagType == BPListFormats::Real32, "Expected Real32, got %i", _tagType);
	return _tagData._float;
}

double BinaryPList::asDouble() const {
	core_assert_msg(_tagType == BPListFormats::Real64, "Expected Real64, got %i", _tagType);
	return _tagData._double;
}

const core::String &BinaryPList::asString() const {
	core_assert_msg(isString(), "Expected String, got %i", _tagType);
	return *_tagData._string;
}

BinaryPList &BinaryPList::operator=(const BinaryPList &val) {
	if (_tagType == BPListFormats::MAX) {
		_tagType = val._tagType;
		_tagData.copy(val._tagType, val._tagData);
	} else if (_tagType == val._tagType) {
		switch (_tagType) {
		case BPListFormats::ASCIIString:
		case BPListFormats::Unicode16String:
			*_tagData._string = *val._tagData._string;
			break;
		case BPListFormats::Dict:
			*_tagData._dict = *val._tagData._dict;
			break;
		case BPListFormats::Array:
		case BPListFormats::Set:
			*_tagData._array = *val._tagData._array;
			break;
		case BPListFormats::Data:
			*_tagData._data = *val._tagData._data;
			break;
		default:
			_tagData = val._tagData;
			break;
		}
	}
	return *this;
}

BinaryPList &BinaryPList::operator=(BinaryPList &&other) noexcept {
	if (this == &other) {
		return *this;
	}
	this->~BinaryPList();

	_tagData = other._tagData;
	_tagType = other._tagType;

	other._tagType = BPListFormats::MAX;
	other._tagData._string = nullptr;
	return *this;
}

BinaryPList::BinaryPList(BinaryPList &&other) noexcept : _tagData(other._tagData), _tagType(other._tagType) {
	other._tagType = BPListFormats::MAX;
	other._tagData._string = nullptr;
}

BinaryPList::BinaryPList(PListDict &&val) : _tagType(BPListFormats::Dict) {
	_tagData._dict = new PListDict(core::move(val));
}

BinaryPList::BinaryPList(PListArray &&val) : _tagType(BPListFormats::Array) {
	_tagData._array = new PListArray(core::move(val));
}

BinaryPList::BinaryPList(PListByteArray &&val) : _tagType(BPListFormats::Data) {
	_tagData._data = new PListByteArray(core::move(val));
}

BinaryPList::BinaryPList(core::String &&val) : _tagType(BPListFormats::ASCIIString) {
	_tagData._string = new core::String(core::move(val));
}

BinaryPList::BinaryPList(const BinaryPList &val) {
	_tagType = val._tagType;
	_tagData.copy(val._tagType, val._tagData);
}

BinaryPList::~BinaryPList() {
	switch (_tagType) {
	case Unicode16String:
	case ASCIIString:
		delete _tagData._string;
		break;
	case Dict:
		delete _tagData._dict;
		break;
	case Array:
	case Set:
		delete _tagData._array;
		break;
	case Data:
		delete _tagData._data;
		break;
	case False:
	case True:
	case UID:
	case Int:
	case Real:
	case Date:
	case Null:
	case Fill:
	case MAX:
	case Int8:
	case Int16:
	case Int32:
	case Int64:
	case Real32:
	case Real64:
		break;
	}
}

const BinaryPList &BinaryPList::getDictEntry(const core::String &id) const {
	static BinaryPList empty;
	if (!isDict()) {
		return empty;
	}
	auto iter = asDict().find(id);
	if (iter == asDict().end()) {
		return empty;
	}
	return iter->value;
}

bool BinaryPList::empty() const {
	if (isDict()) {
		return asDict().empty();
	}
	if (isString()) {
		return asString().empty();
	}
	if (isArray()) {
		return asArray().empty();
	}
	if (isData()) {
		return asData().empty();
	}
	return false;
}

size_t BinaryPList::size() const {
	if (isDict()) {
		return asDict().size();
	}
	if (isString()) {
		return asString().size();
	}
	if (isArray()) {
		return asArray().size();
	}
	if (isData()) {
		return asData().size();
	}
	return 0u;
}

bool BinaryPList::parseHeader(io::SeekableReadStream &stream) {
	char header[8];
	if (stream.read(header, sizeof(header)) == -1) {
		Log::error("Failed to read the header");
		return false;
	}
	if (core_memcmp(header, "bplist", 6) != 0) {
		Log::error("Invalid header");
		return false;
	}
	if (header[6] != '0' || header[7] != '0') {
		Log::error("Invalid version %c", header[6]);
		return false;
	}
	Log::debug("Binary PList version %c", header[7]);
	return true;
}

BPListTrailer BinaryPList::parseTrailer(io::SeekableReadStream &stream) {
	BPListTrailer trailer;
	int64_t pos = stream.pos();
	if (stream.seek(-32, SEEK_END) == -1) {
		Log::error("Failed to seek to the end of the stream to read the trailer data");
		return trailer;
	}
	if (stream.read(trailer.unused, sizeof(trailer.unused)) == -1) {
		Log::error("Failed to read the trailer data");
		return trailer;
	}
	if (stream.readUInt8(trailer.version) != 0) {
		Log::error("Failed to read the version");
		return trailer;
	}
	if (stream.readUInt8(trailer.offsetIntSize) != 0) {
		Log::error("Failed to read the offsetIntSize");
		return trailer;
	}
	if (stream.readUInt8(trailer.objectRefSize) != 0) {
		Log::error("Failed to read the objectRefSize");
		return trailer;
	}
	if (stream.readUInt64BE(trailer.numObjects) != 0) {
		Log::error("Failed to read the numObjects");
		return trailer;
	}
	if (stream.readUInt64BE(trailer.topObject) != 0) {
		Log::error("Failed to read the topObject");
		return trailer;
	}
	if (stream.readUInt64BE(trailer.offsetTableOffset) != 0) {
		Log::error("Failed to read the offsetTableOffset");
		return trailer;
	}
	if (stream.seek(pos, SEEK_SET) == -1) {
		Log::error("Failed to seek back to the original stream position");
		return trailer;
	}
	if (trailer.numObjects <= 0 || trailer.objectRefSize == 0 || trailer.offsetIntSize == 0 ||
		trailer.topObject >= trailer.numObjects) {
		Log::error("Header validation failed");
		return trailer;
	}
	trailer.valid = true;
	return trailer;
}

bool BinaryPList::readObject(io::SeekableReadStream &stream, BPListFormats &object) {
	if (stream.remaining() <= 32) {
		return false;
	}
	return stream.read(&object, sizeof(object)) == sizeof(object);
}

uint64_t BinaryPList::readSizedInt(io::SeekableReadStream &stream, uint8_t numberBytes) {
	if (numberBytes == 1) {
		uint8_t data;
		stream.readUInt8(data);
		return data;
	} else if (numberBytes == 2) {
		uint16_t data;
		stream.readUInt16BE(data);
		return data;
	} else if (numberBytes == 4) {
		uint32_t data;
		stream.readUInt32BE(data);
		return data;
	}
	core_assert_msg(numberBytes == 8, "Invalid number of bytes: %i", (int)numberBytes);
	uint64_t data;
	stream.readUInt64BE(data);
	return data;
}

uint32_t BinaryPList::readLength(io::SeekableReadStream &stream, BPListFormats nibble) {
	if (nibble == BPListFormats::Fill) {
		BPListFormats object;
		if (!readObject(stream, object)) {
			Log::error("Failed to read type for length");
			return UINT32_MAX;
		}
		const BPListFormats type = (BPListFormats)(object & 0xF0);
		if (type != BPListFormats::Int) {
			Log::error("Unexpected type for length: %i", (int)type);
			return UINT32_MAX;
		}
		const BPListFormats size = (BPListFormats)(object & BPListFormats::Fill);
		return readSizedInt(stream, 1 << size);
	}
	return nibble;
}

BinaryPList BinaryPList::readUID(io::SeekableReadStream &stream, BPListFormats size) {
	uint64_t numberBytes = size + 1;
	Log::debug("BPLIST: Read uid with %i bytes", (int)numberBytes);
	if (numberBytes == 1) {
		uint8_t data;
		stream.readUInt8(data);
		return BinaryPList{BPListFormats::UID, data};
	} else if (numberBytes == 2) {
		uint16_t data;
		stream.readUInt16BE(data);
		return BinaryPList{BPListFormats::UID, data};
	} else if (numberBytes == 4) {
		uint32_t data;
		stream.readUInt32BE(data);
		return BinaryPList{BPListFormats::UID, data};
	} else if (numberBytes == 8) {
		uint64_t data;
		stream.readUInt64BE(data);
		return BinaryPList{BPListFormats::UID, data};
	}
	Log::error("Can't read uid int with %i bytes", (int)numberBytes);
	return BinaryPList{};
}

BinaryPList BinaryPList::readDate(io::SeekableReadStream &stream) {
	Log::debug("BPLIST: Read date");
	double date;
	stream.readDoubleBE(date);
	return BinaryPList{BPListFormats::Date, date};
}

BinaryPList BinaryPList::readInt(io::SeekableReadStream &stream, BPListFormats size) {
	const uint64_t numberBytes = (uint64_t)1 << size;
	Log::debug("BPLIST: Read int with %i bytes", (int)numberBytes);
	if (numberBytes == 1) {
		uint8_t data;
		stream.readUInt8(data);
		return BinaryPList{data};
	} else if (numberBytes == 2) {
		uint16_t data;
		stream.readUInt16BE(data);
		return BinaryPList{data};
	} else if (numberBytes == 4) {
		uint32_t data;
		stream.readUInt32BE(data);
		return BinaryPList{data};
	} else if (numberBytes == 8) {
		uint64_t data;
		stream.readUInt64BE(data);
		return BinaryPList{data};
	}
	Log::error("Can't read int with %i bytes", (int)numberBytes);
	return BinaryPList{};
}

BinaryPList BinaryPList::readReal(io::SeekableReadStream &stream, BPListFormats type) {
	Log::debug("BPLIST: Read real of type %i", (int)type);
	if (type == 2) {
		float val;
		if (stream.readFloatBE(val) != 0) {
			Log::error("Failed to read float");
			return BinaryPList{};
		}
		return BinaryPList{val};
	} else if (type == 3) {
		double val;
		if (stream.readDoubleBE(val) != 0) {
			Log::error("Failed to read double");
			return BinaryPList{};
		}
		return BinaryPList{val};
	}
	Log::error("Can't read real number with %i as id", (int)type);
	return BinaryPList{};
}

BinaryPList BinaryPList::readUTF16Str(io::SeekableReadStream &stream, BPListFormats size) {
	const uint32_t length = readLength(stream, size);
	Log::debug("BPLIST: Read utf16 string of length %u", length);
	core::String str;
	if (!stream.readUTF16BE(length, str)) {
		Log::error("Failed to read or convert string");
		return BinaryPList{};
	}
	Log::debug("Read string %s", str.c_str());
	return BinaryPList{core::move(str)};
}

BinaryPList BinaryPList::readData(io::SeekableReadStream &stream, BPListFormats size) {
	const uint32_t length = readLength(stream, size);
	Log::debug("BPLIST: Read data of length %u", length);
	PListByteArray data;
	data.resize(length);
	if (!stream.read(data.data(), data.size())) {
		Log::error("Failed to read data of length %i", (int)length);
		return BinaryPList{};
	}
	return BinaryPList{core::move(data)};
}

BinaryPList BinaryPList::readString(io::SeekableReadStream &stream, BPListFormats size) {
	const uint32_t length = readLength(stream, size);
	Log::debug("BPLIST: Read string of length %u", length);
	core::String str;
	if (!stream.readString((int)length, str, false)) {
		Log::error("Failed to read string of length %i", (int)length);
		return BinaryPList{};
	}
	Log::debug("Read string %s", str.c_str());
	return BinaryPList{core::move(str)};
}

uint64_t BinaryPList::readOffset(io::SeekableReadStream &stream, const BPListTrailer &trailer) {
	const uint64_t ref = readSizedInt(stream, trailer.objectRefSize);
	if (ref >= trailer.numObjects) {
		Log::error("Invalid object reference %i", (int)ref);
		return UINT64_MAX;
	}
	const int64_t pos = stream.pos();
	const int64_t offsetTableOffset = (int64_t)(trailer.offsetTableOffset + ref * trailer.offsetIntSize);
	if (stream.seek(offsetTableOffset) == -1) {
		Log::error("Failed to seek to offset table");
		return UINT64_MAX;
	}
	const uint64_t off = readSizedInt(stream, trailer.offsetIntSize);
	if (stream.seek(pos) == -1) {
		Log::error("Failed to seek back to original position after reading offset");
		return UINT64_MAX;
	}
	return off;
}

BinaryPList BinaryPList::readArrayAndSet(io::SeekableReadStream &stream, BPListFormats size, BPListState &state) {
	uint32_t arrayCount = readLength(stream, size);
	Log::debug("BPLIST: Read array or set with %u elements", arrayCount);

	PListArray array;
	array.reserve(arrayCount);

	for (uint32_t i = 0u; i < arrayCount; ++i) {
		const uint64_t offset = readOffset(stream, state.trailer);
		if (offset == UINT64_MAX) {
			Log::error("Failed to get offset for array element %u", i);
			return BinaryPList{};
		}
		const int64_t pos = stream.pos();
		if (stream.seek((int64_t)offset, SEEK_SET) == -1) {
			Log::error("Failed to seek to offset from offset table for element %u", i);
			return BinaryPList{};
		}
		array.emplace_back(parse(stream, state));
		if (stream.seek(pos) == -1) {
			Log::error("Failed to seek back to original position after reading offset for element %u", i);
			return BinaryPList{};
		}
		Log::debug("Finished reading array entry %i", i);
	}
	return BinaryPList{core::move(array)};
}

BinaryPList BinaryPList::readDict(io::SeekableReadStream &stream, BPListFormats size, BPListState &state) {
	const uint32_t entryCount = readLength(stream, size);
	Log::debug("# BPLIST: Read dict: %u entries", entryCount);

	PListDict dict;

	core::DynamicArray<core::String> plistKeys;
	plistKeys.reserve(entryCount);

	for (uint32_t i = 0u; i < entryCount; ++i) {
		const uint64_t offset = readOffset(stream, state.trailer);
		if (offset == UINT64_MAX) {
			Log::error("Failed to get offset for dict element %u", i);
			return BinaryPList{};
		}
		const int64_t pos = stream.pos();
		if (stream.seek((int64_t)offset, SEEK_SET) == -1) {
			Log::error("Failed to seek to offset from offset table for element %u", i);
			return BinaryPList{};
		}
		BinaryPList plistKey = parse(stream, state);
		if (plistKey.type() != BPListFormats::ASCIIString) {
			Log::error("Invalid key type for dict entry at %u", i);
			return BinaryPList{};
		}
		plistKeys.push_back(plistKey.asString());
		if (stream.seek(pos) == -1) {
			Log::error("Failed to seek back to original position after reading offset for element %u", i);
			return BinaryPList{};
		}
	}
	for (uint32_t i = 0u; i < entryCount; ++i) {
		const uint64_t offset = readOffset(stream, state.trailer);
		if (offset == UINT64_MAX) {
			Log::error("Failed to get offset for dict element %u", i);
			return BinaryPList{};
		}
		const int64_t pos = stream.pos();
		if (stream.seek((int64_t)offset, SEEK_SET) == -1) {
			Log::error("Failed to seek to offset from offset table for element %u", i);
			return BinaryPList{};
		}
		BinaryPList plistValue = parse(stream, state);
		dict.emplace(plistKeys[i], core::move(plistValue));
		if (stream.seek(pos) == -1) {
			Log::error("Failed to seek back to original position after reading offset for element %u", i);
			return BinaryPList{};
		}
	}

	Log::debug("# Dict end with %i entries", (int)dict.size());
	return BinaryPList{core::move(dict)};
}

BinaryPList BinaryPList::readNull() {
	Log::debug("BPLIST: Read null");
	return BinaryPList{BPListFormats::Null, (uint64_t)0};
}

BinaryPList BinaryPList::readBool(bool value) {
	Log::debug("BPLIST: Read bool: %s", value ? "true" : "false");
	return BinaryPList{value};
}

BinaryPList BinaryPList::parse(io::SeekableReadStream &stream) {
	if (stream.pos() != 0) {
		Log::error("Stream must be at the beginning");
		return BinaryPList{};
	}
	if (!parseHeader(stream)) {
		Log::error("Failed to parse plist header");
		return BinaryPList{};
	}
	BPListState state;
	state.trailer = parseTrailer(stream);
	if (!state.trailer.valid) {
		Log::error("Failed to parse plist data");
		return BinaryPList{};
	}

	const int64_t topLevelOffset =
		(int64_t)(state.trailer.offsetTableOffset + state.trailer.topObject * state.trailer.offsetIntSize);
	if (stream.seek(topLevelOffset, SEEK_SET) == -1) {
		Log::error("Failed to seek to top level element");
		return BinaryPList{};
	}
	const uint64_t off = readSizedInt(stream, state.trailer.offsetIntSize);
	if (off == UINT64_MAX) {
		Log::error("Failed to read top level element offset");
		return BinaryPList{};
	}
	if (stream.seek((int64_t)off) == -1) {
		Log::error("Failed to seek to top level element offset");
		return UINT64_MAX;
	}

	return parse(stream, state);
}

BinaryPList BinaryPList::parse(io::SeekableReadStream &stream, BPListState &state) {
	BPListFormats object;
	if (!readObject(stream, object)) {
		Log::error("Failed to read type");
		return BinaryPList{};
	}
	++state.objects;
	const BPListFormats type = (BPListFormats)(object & 0xF0);
	const BPListFormats size = (BPListFormats)(object & BPListFormats::Fill);
	switch (type) {
	case BPListFormats::True:
		return readBool(true);
	case BPListFormats::False:
		return readBool(false);
	case BPListFormats::Null:
		return readNull();
	case BPListFormats::Real:
		return readReal(stream, size);
	case BPListFormats::Int:
		return readInt(stream, size);
	case BPListFormats::Date:
		return readDate(stream);
	case BPListFormats::UID:
		return readUID(stream, size);
	case BPListFormats::Unicode16String:
		return readUTF16Str(stream, size);
	case BPListFormats::Data:
		return readData(stream, size);
	case BPListFormats::ASCIIString:
		return readString(stream, size);
	case BPListFormats::Array:
	case BPListFormats::Set:
		return readArrayAndSet(stream, size, state);
	case BPListFormats::Dict:
		return readDict(stream, size, state);

	// unused or custom types
	case BPListFormats::Int8:
	case BPListFormats::Int16:
	case BPListFormats::Int32:
	case BPListFormats::Int64:
	case BPListFormats::Real32:
	case BPListFormats::Real64:
	case BPListFormats::Fill:
	case BPListFormats::MAX:
		break;
	}
	Log::error("BPLIST: Unknown type: %i", (int)object);
	return BinaryPList{};
}

} // namespace util
