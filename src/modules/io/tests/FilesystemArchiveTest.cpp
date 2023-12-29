/**
 * @file
 */

#include "core/tests/TestHelper.h"
#include "io/Filesystem.h"
#include "io/FilesystemArchive.h"
#include <gtest/gtest.h>

namespace io {

class FilesystemArchiveTest : public testing::Test {
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

TEST_F(FilesystemArchiveTest, testFilesytemArchiveShortNames) {
	io::FilesystemArchive fsa(fs, false);
    fsa.add(".");
    ASSERT_FALSE(fsa.files().empty());
    SeekableReadStreamPtr stream = fsa.readStream(fsa.files().front().fullPath);
    ASSERT_TRUE(stream);
    EXPECT_TRUE(fsa.exists("iotest.txt"));
}

} // namespace io
