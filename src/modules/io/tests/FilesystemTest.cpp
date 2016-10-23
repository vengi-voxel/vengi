#include "core/tests/AbstractTest.h"
#include "io/Filesystem.h"

namespace io {

class FilesystemTest: public core::AbstractTest {
};

TEST_F(FilesystemTest, testListDirectory) {
	io::Filesystem fs;
	std::vector<io::Filesystem::DirEntry> entities;
	fs.list(".", entities, "*");
	ASSERT_FALSE(entities.empty());
}

}
