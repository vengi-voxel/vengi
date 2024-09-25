#include "app/tests/AbstractTest.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"
#include <gtest/gtest.h>

namespace io {

class FileTest : public app::AbstractTest {};

TEST_F(FileTest, testIsAnyOf) {
	io::Filesystem fs;
	fs.init("test", "test");

	const io::FilePtr &img = fs.open("image.png");
	EXPECT_TRUE(img->isAnyOf(io::format::images()));
	EXPECT_FALSE(img->isAnyOf(io::format::lua()));
}

TEST_F(FileTest, testGetPath) {
	io::Filesystem fs;
	fs.init("test", "test");
	const io::FilePtr &file = fs.open("foobar/1.txt", io::FileMode::Read);
	EXPECT_TRUE(core::string::endsWith(file->dir(), "foobar/"));
	EXPECT_EQ("txt", file->extension());
	EXPECT_EQ("1", file->fileName());
	EXPECT_TRUE(core::string::endsWith(file->name(), "foobar/1.txt"));
	EXPECT_FALSE(file->exists());
}

TEST_F(FileTest, testLoad) {
	io::Filesystem fs;
	fs.init("test", "test");
	const io::FilePtr &file = fs.open("iotest.txt", io::FileMode::Read);
	const core::String &str = file->load();
	EXPECT_NE("", str);
}

} // namespace io
