/**
 * @file
 */

#include "io/FilesystemArchive.h"
#include "app/tests/AbstractTest.h"
#include "core/tests/TestHelper.h"
#include "io/Filesystem.h"
#include <gtest/gtest.h>

namespace io {

class FilesystemArchiveTest : public app::AbstractTest {
protected:
	io::FilesystemPtr fs;

public:
	void SetUp() override {
		fs = core::make_shared<io::Filesystem>();
		EXPECT_TRUE(fs->init("test", "test")) << "Failed to initialize the filesystem";
	}

	void TearDown() override {
		fs->shutdown();
		fs.release();
	}
};

TEST_F(FilesystemArchiveTest, testFilesytemArchiveCurrentDir) {
	io::FilesystemArchive fsa(fs);
	fsa.init(".");
	ASSERT_FALSE(fsa.files().empty());
	FilesystemEntry entry = fsa.files().front();
	ASSERT_TRUE(fsa.readStream(entry.fullPath)) << "Should be able to read a file with a full path";
	ASSERT_TRUE(fsa.readStream(entry.name))
		<< "Should be able to read a file with just the name because the archive was for the current working dir";
	EXPECT_TRUE(fsa.exists("iotest.txt"));
}

TEST_F(FilesystemArchiveTest, testFilesytemArchiveNoDir) {
	io::FilesystemArchive fsa(fs);
	EXPECT_TRUE(fsa.exists("iotest.txt"));
}

} // namespace io
