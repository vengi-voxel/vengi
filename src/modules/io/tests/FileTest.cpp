#include "core/tests/AbstractTest.h"
#include "io/Filesystem.h"

namespace io {

class FileTest: public core::AbstractTest {
};

TEST_F(FileTest, testGetPath) {
	io::Filesystem fs;
	fs.init("test", "test");
	const io::FilePtr& file = fs.open("foobar/1.txt", io::FileMode::Read);
	ASSERT_EQ("foobar", file->path());
	ASSERT_EQ("txt", file->extension());
	ASSERT_EQ("1", file->fileName());
	ASSERT_EQ("foobar/1.txt", file->name());
	ASSERT_FALSE(file->exists());
}

}
