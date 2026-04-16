/**
 * @file
 */

#include "io/StreamArchive.h"
#include "core/ScopedPtr.h"
#include "io/BufferedReadWriteStream.h"
#include <gtest/gtest.h>

namespace io {

class StreamArchiveTest : public testing::Test {};

TEST_F(StreamArchiveTest, testReadStream) {
	uint8_t buf[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	io::BufferedReadWriteStream stream(sizeof(buf));
	stream.write(buf, sizeof(buf));
	stream.seek(0);
	StreamArchive archive((io::SeekableReadStream *)&stream);
	core::ScopedPtr<io::SeekableReadStream> rs(archive.readStream("any"));
	ASSERT_TRUE(rs);
	uint8_t readBuf[sizeof(buf)];
	ASSERT_EQ(rs->read(readBuf, sizeof(readBuf)), (int64_t)sizeof(buf));
	ASSERT_EQ(core_memcmp(buf, readBuf, sizeof(buf)), 0);
}

TEST_F(StreamArchiveTest, testWriteStream) {
	io::BufferedReadWriteStream stream(512);
	StreamArchive archive((io::SeekableWriteStream *)&stream);
	core::ScopedPtr<io::SeekableWriteStream> ws(archive.writeStream("any"));
	ASSERT_TRUE(ws);
	uint8_t buf[] = {0, 1, 2, 3};
	ASSERT_NE(ws->write(buf, sizeof(buf)), -1);
}

TEST_F(StreamArchiveTest, testReadStreamRewinds) {
	uint8_t buf[] = {10, 20, 30};
	io::BufferedReadWriteStream stream(sizeof(buf));
	stream.write(buf, sizeof(buf));
	stream.seek(0);
	StreamArchive archive((io::SeekableReadStream *)&stream);
	core::ScopedPtr<io::SeekableReadStream> rs1(archive.readStream("first"));
	ASSERT_TRUE(rs1);
	uint8_t readBuf[sizeof(buf)];
	ASSERT_EQ(rs1->read(readBuf, sizeof(readBuf)), (int64_t)sizeof(buf));

	core::ScopedPtr<io::SeekableReadStream> rs2(archive.readStream("second"));
	ASSERT_TRUE(rs2);
	uint8_t readBuf2[sizeof(buf)];
	ASSERT_EQ(rs2->read(readBuf2, sizeof(readBuf2)), (int64_t)sizeof(buf));
	ASSERT_EQ(core_memcmp(buf, readBuf2, sizeof(buf)), 0);
}

TEST_F(StreamArchiveTest, testFactory) {
	io::BufferedReadWriteStream stream(64);
	io::ArchivePtr archive = io::openStreamArchive((io::SeekableReadStream *)&stream);
	ASSERT_TRUE(archive);
}

} // namespace io
