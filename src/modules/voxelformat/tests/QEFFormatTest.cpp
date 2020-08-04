/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QEFFormat.h"
#include "voxelformat/Loader.h"

namespace voxel {

class QEFFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QEFFormatTest, testLoad) {
	QEFFormat f;
	std::unique_ptr<RawVolume> volume(load("qubicle.qef", f));
	ASSERT_NE(nullptr, volume) << "Could not load qef file";
}

TEST_F(QEFFormatTest, testSaveSmallVoxel) {
	QEFFormat f;
	Region region(glm::ivec3(0), glm::ivec3(1));
	RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 245)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, createVoxel(VoxelType::Generic, 127)));
	ASSERT_TRUE(original.setVoxel(0, 1, 0, createVoxel(VoxelType::Generic, 200)));
	ASSERT_TRUE(f.save(&original, open("qubicle-smallvolumesavetest.qef", io::FileMode::Write)));
	f = QEFFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("qubicle-smallvolumesavetest.qef")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

}
