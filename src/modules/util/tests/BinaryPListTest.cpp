/**
 * @file
 */

#include "util/BinaryPList.h"
#include "app/App.h"
#include "app/tests/AbstractTest.h"
#include "io/FileStream.h"
#include "io/BufferedReadWriteStream.h"

namespace util {

class BinaryPListTest : public app::AbstractTest {
protected:
	static void writeSizedInt(io::WriteStream &stream, uint64_t value, uint8_t width) {
		for (int byte = width - 1; byte >= 0; --byte) {
			stream.writeUInt8((uint8_t)(value >> (byte * 8)));
		}
	}

	static void writeArray(io::BufferedReadWriteStream &stream, uint8_t offsetWidth, uint8_t refWidth, int padding = 0) {
		stream.write("bplist00", 8);
		for (int i = 0; i < padding; ++i) {
			stream.writeUInt8(0);
		}
		const uint64_t rootOffset = stream.pos();
		stream.writeUInt8(0xa2); // array containing object references 1 and 2
		writeSizedInt(stream, 1, refWidth);
		writeSizedInt(stream, 2, refWidth);
		const uint64_t stringOffset = stream.pos();
		stream.writeUInt8(0x52);
		stream.write("ok", 2);
		const uint64_t intOffset = stream.pos();
		stream.writeUInt8(0x11);
		stream.writeUInt16BE(0x0102);
		const uint64_t tableOffset = stream.pos();
		writeSizedInt(stream, rootOffset, offsetWidth);
		writeSizedInt(stream, stringOffset, offsetWidth);
		writeSizedInt(stream, intOffset, offsetWidth);
		for (int i = 0; i < 6; ++i) {
			stream.writeUInt8(0);
		}
		stream.writeUInt8(offsetWidth);
		stream.writeUInt8(refWidth);
		stream.writeUInt64BE(3);
		stream.writeUInt64BE(0);
		stream.writeUInt64BE(tableOffset);
		stream.seek(0);
	}

	void checkArray(io::SeekableReadStream &stream) {
		const BinaryPList plist = BinaryPList::parse(stream);
		ASSERT_TRUE(plist.isArray());
		ASSERT_EQ(2u, plist.asArray().size());
		ASSERT_TRUE(plist.asArray()[0].isString());
		EXPECT_EQ("ok", plist.asArray()[0].asString());
		ASSERT_TRUE(plist.asArray()[1].isInt());
		EXPECT_EQ(0x0102u, plist.asArray()[1].asInt());
	}
};

TEST_F(BinaryPListTest, testRead) {
	io::FileStream stream(io::filesystem()->open("test.plist", io::FileMode::Read));
	const util::BinaryPList &plist = util::BinaryPList::parse(stream);
	ASSERT_TRUE(plist.isDict());
	ASSERT_EQ(3u, plist.asDict().size());

	auto travelLog = plist.asDict().find("Travel Log");
	ASSERT_NE(travelLog, plist.asDict().end());
	ASSERT_TRUE(travelLog->value.isArray());
	const util::PListArray &travelLogArray = travelLog->value.asArray();
	ASSERT_EQ(3u, travelLogArray.size());
	ASSERT_TRUE(travelLogArray[0].isString());
	ASSERT_TRUE(travelLogArray[1].isString());
	ASSERT_TRUE(travelLogArray[2].isString());
	ASSERT_STREQ("Tokyo, Honshu, Japan", travelLogArray[0].asString().c_str());
	ASSERT_STREQ("Philadelphia, PA", travelLogArray[1].asString().c_str());
	ASSERT_STREQ("Recife, Pernambuco, Brazil", travelLogArray[2].asString().c_str());

	auto birthYear = plist.asDict().find("Birth Year");
	ASSERT_NE(birthYear, plist.asDict().end());
	ASSERT_TRUE(birthYear->second.isInt());
	ASSERT_EQ(1942u, birthYear->second.asInt());

	auto name = plist.asDict().find("Name");
	ASSERT_NE(name, plist.asDict().end());
	ASSERT_STREQ("John Doe", name->second.asString().c_str());
}

TEST_F(BinaryPListTest, testReadVMaxPalette) {
	io::FileStream stream(io::filesystem()->open("palette.settings.vmaxpsb", io::FileMode::Read));
	const util::BinaryPList &plist = util::BinaryPList::parse(stream);
	ASSERT_TRUE(plist.isDict());
	ASSERT_EQ(11u, plist.asDict().size());
	ASSERT_TRUE(plist.asDict().hasKey("materials"));
	ASSERT_TRUE(plist.asDict().hasKey("name"));
}

TEST_F(BinaryPListTest, testAllOffsetAndReferenceWidths) {
	// Unlike integer object payloads, these widths need not be powers of two.
	for (uint8_t offsetWidth = 1; offsetWidth <= 8; ++offsetWidth) {
		for (uint8_t refWidth = 1; refWidth <= 8; ++refWidth) {
			SCOPED_TRACE(offsetWidth);
			SCOPED_TRACE(refWidth);
			io::BufferedReadWriteStream stream;
			writeArray(stream, offsetWidth, refWidth);
			checkArray(stream);
		}
	}
}

TEST_F(BinaryPListTest, testThreeByteOffsetsAbove64K) {
	io::BufferedReadWriteStream stream;
	writeArray(stream, 3, 1, 65536);
	checkArray(stream);
}

TEST_F(BinaryPListTest, testRejectInvalidTrailerWidths) {
	const uint8_t widths[] = {0, 9, 255};
	for (uint8_t width : widths) {
		for (int field = 0; field < 2; ++field) {
			SCOPED_TRACE(width);
			SCOPED_TRACE(field);
			io::BufferedReadWriteStream stream;
			writeArray(stream, 1, 1);
			stream.seek(stream.size() - 26 + field);
			stream.writeUInt8(width);
			stream.seek(0);
			EXPECT_EQ(BPListFormats::MAX, BinaryPList::parse(stream).type());
		}
	}
}

TEST_F(BinaryPListTest, testRejectTruncatedSizedOffset) {
	io::BufferedReadWriteStream stream;
	writeArray(stream, 3, 1);
	// Point at the last byte so the three-byte offset cannot be read in full.
	const uint64_t truncatedOffset = stream.size() - 1;
	stream.seek(stream.size() - 8);
	stream.writeUInt64BE(truncatedOffset);
	stream.seek(0);
	EXPECT_EQ(BPListFormats::MAX, BinaryPList::parse(stream).type());
}

} // namespace util
