/**
 * @file
 */

#include "io/FilesystemArchive.h"
#include "app/tests/AbstractTest.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "io/Archive.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include <gtest/gtest.h>

namespace io {

class FilesystemArchiveTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	io::FilesystemPtr fs;

public:
	void SetUp() override {
		Super::SetUp();
		fs = core::make_shared<io::Filesystem>();
		EXPECT_TRUE(fs->init("test", "test")) << "Failed to initialize the filesystem";
	}

	void TearDown() override {
		fs->shutdown();
		fs.release();
		Super::TearDown();
	}
};

// TODO: fix me in pipeline
TEST_F(FilesystemArchiveTest, DISABLED_testFilesytemArchiveCurrentDir) {
	io::FilesystemArchive fsa(fs);
	fsa.init(".");
	ASSERT_FALSE(fsa.files().empty());
	FilesystemEntry entry = fsa.files().front();
	core::ScopedPtr<io::SeekableReadStream> rs(fsa.readStream(entry.fullPath));
	ASSERT_TRUE(rs) << "Should be able to read a file with a full path";
	core::ScopedPtr<io::SeekableReadStream> rs2(fsa.readStream(entry.name));
	ASSERT_TRUE(rs2)
		<< "Should be able to read a file with just the name because the archive was for the current working dir";
	EXPECT_TRUE(fsa.exists("iotest.txt"));
}

TEST_F(FilesystemArchiveTest, testFilesytemArchiveNoDir) {
	io::FilesystemArchive fsa(fs);
	EXPECT_TRUE(fsa.exists("iotest.txt"));
}

TEST_F(FilesystemArchiveTest, testFilesytemArchiveList) {
	io::FilesystemArchive fsa(fs);
	io::ArchiveFiles files;
	fsa.list("iotest.txt", files);
	ASSERT_EQ(files.size(), 1u);
	EXPECT_EQ(files.front().name, "iotest.txt");
	fsa.list("", files, "iotest.txt");
	ASSERT_EQ(files.size(), 2u);
	EXPECT_EQ(files.back().name, "iotest.txt");
}

} // namespace io
