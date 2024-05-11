/**
 * @file
 */

#include "io/MemoryArchive.h"
#include "core/ScopedPtr.h"
#include "io/Stream.h"
#include <gtest/gtest.h>

namespace io {

class MemoryArchiveTest : public testing::Test {};

TEST_F(MemoryArchiveTest, testMemoryArchiveAdd) {
	io::MemoryArchive a;
	uint8_t buf[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	ASSERT_TRUE(a.add("test", buf, sizeof(buf)));
	ASSERT_FALSE(a.add("test", buf, sizeof(buf))) << "a file with the same name should already exists";
	core::ScopedPtr<io::SeekableReadStream> stream(a.readStream("test"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(stream->size(), sizeof(buf));
}

TEST_F(MemoryArchiveTest, testMemoryArchiveAddViaWrite) {
	io::MemoryArchive a;
	uint8_t buf[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	core::ScopedPtr<io::SeekableWriteStream> w(a.writeStream("test"));
	ASSERT_NE(w->write(buf, sizeof(buf)), -1);
	ASSERT_FALSE(a.add("test", buf, sizeof(buf))) << "a file with the same name should already exists";
	core::ScopedPtr<io::SeekableReadStream> stream(a.readStream("test"));
	ASSERT_TRUE(stream);
	EXPECT_EQ(stream->size(), sizeof(buf));
}

} // namespace io
