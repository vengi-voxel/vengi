#include "core/tests/AbstractTest.h"
#include "io/Filesystem.h"

namespace io {

class FileTest: public core::AbstractTest {
};

TEST_F(FileTest, testGetPath) {
	io::File file("foobar/1.txt");
	ASSERT_EQ("foobar", file.getPath());
	ASSERT_EQ("txt", file.getExtension());
	ASSERT_EQ("1", file.getFileName());
	ASSERT_EQ("foobar/1.txt", file.getName());
}

}
