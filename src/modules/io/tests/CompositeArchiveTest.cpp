/**
 * @file
 */

#include "io/CompositeArchive.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "core/SharedPtr.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include <gtest/gtest.h>

namespace io {

class CompositeArchiveTest : public app::AbstractTest {};

TEST_F(CompositeArchiveTest, testReadFromFirstArchive) {
	MemoryArchivePtr a1 = openMemoryArchive();
	const uint8_t data[] = {'h', 'e', 'l', 'l', 'o'};
	a1->add("file1.txt", data, sizeof(data));

	MemoryArchivePtr a2 = openMemoryArchive();

	CompositeArchive composite;
	composite.add(a1);
	composite.add(a2);

	EXPECT_TRUE(composite.exists("file1.txt"));
	core::ScopedPtr<SeekableReadStream> stream(composite.readStream("file1.txt"));
	ASSERT_TRUE(stream);
}

TEST_F(CompositeArchiveTest, testReadFromSecondArchive) {
	MemoryArchivePtr a1 = openMemoryArchive();

	MemoryArchivePtr a2 = openMemoryArchive();
	const uint8_t data[] = {'w', 'o', 'r', 'l', 'd'};
	a2->add("file2.txt", data, sizeof(data));

	CompositeArchive composite;
	composite.add(a1);
	composite.add(a2);

	EXPECT_TRUE(composite.exists("file2.txt"));
	core::ScopedPtr<SeekableReadStream> stream(composite.readStream("file2.txt"));
	ASSERT_TRUE(stream);
}

TEST_F(CompositeArchiveTest, testFirstArchiveHasPriority) {
	const uint8_t data1[] = {'A'};
	const uint8_t data2[] = {'B'};

	MemoryArchivePtr a1 = openMemoryArchive();
	a1->add("shared.txt", data1, sizeof(data1));

	MemoryArchivePtr a2 = openMemoryArchive();
	a2->add("shared.txt", data2, sizeof(data2));

	CompositeArchive composite;
	composite.add(a1);
	composite.add(a2);

	core::ScopedPtr<SeekableReadStream> stream(composite.readStream("shared.txt"));
	ASSERT_TRUE(stream);
	uint8_t buf;
	ASSERT_EQ(stream->readUInt8(buf), 0);
	EXPECT_EQ(buf, 'A') << "First archive should have priority";
}

TEST_F(CompositeArchiveTest, testExistsReturnsFalseForMissing) {
	MemoryArchivePtr a1 = openMemoryArchive();
	MemoryArchivePtr a2 = openMemoryArchive();

	CompositeArchive composite;
	composite.add(a1);
	composite.add(a2);

	EXPECT_FALSE(composite.exists("nonexistent.txt"));
	core::ScopedPtr<SeekableReadStream> stream(composite.readStream("nonexistent.txt"));
	EXPECT_FALSE(stream);
}

TEST_F(CompositeArchiveTest, testListMergesAllArchives) {
	const uint8_t data[] = {'x'};

	MemoryArchivePtr a1 = openMemoryArchive();
	a1->add("a.txt", data, sizeof(data));

	MemoryArchivePtr a2 = openMemoryArchive();
	a2->add("b.txt", data, sizeof(data));

	CompositeArchive composite;
	composite.add(a1);
	composite.add(a2);

	ArchiveFiles files;
	composite.list("", files, "*.txt");
	EXPECT_EQ(files.size(), 2u);
}

TEST_F(CompositeArchiveTest, testShutdownClearsArchives) {
	const uint8_t data[] = {'x'};

	MemoryArchivePtr a1 = openMemoryArchive();
	a1->add("file.txt", data, sizeof(data));

	CompositeArchive composite;
	composite.add(a1);
	EXPECT_TRUE(composite.exists("file.txt"));

	composite.shutdown();
	EXPECT_FALSE(composite.exists("file.txt"));
}

} // namespace io
