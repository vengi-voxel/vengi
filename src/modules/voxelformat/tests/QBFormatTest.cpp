/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QBFormat.h"

namespace voxel {

class QBFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBFormatTest, testLoad) {
	QBFormat f;
	std::unique_ptr<RawVolume> volume(load("qubicle.qb", f));

	VolumePrintThreshold = 40;
	ASSERT_NE(nullptr, volume) << "Could not load qb file";

	// feet
	ASSERT_NE(Empty, volume->voxel(18, 0, 1)) << *volume;
	ASSERT_NE(Empty, volume->voxel(18, 0, 2)) << *volume;
	ASSERT_EQ(Empty, volume->voxel(18, 0, 3)) << *volume;
	ASSERT_EQ(Empty, volume->voxel(18, 0, 4)) << *volume;
	ASSERT_NE(Empty, volume->voxel(22, 0, 1)) << *volume;
	ASSERT_NE(Empty, volume->voxel(22, 0, 2)) << *volume;
	ASSERT_EQ(Empty, volume->voxel(22, 0, 3)) << *volume;
	ASSERT_EQ(Empty, volume->voxel(22, 0, 4)) << *volume;
}

TEST_F(QBFormatTest, testSaveSmallVoxel) {
	QBFormat f;
	Region region(glm::ivec3(0), glm::ivec3(1));
	RawVolume original(region);
	original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1));
	original.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 245));
	original.setVoxel(0, 1, 1, createVoxel(VoxelType::Generic, 127));
	original.setVoxel(0, 1, 0, createVoxel(VoxelType::Generic, 200));
	ASSERT_TRUE(f.save(&original, open("qubicle-smallvolumesavetest.qb", io::FileMode::Write)));
	f = QBFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("qubicle-smallvolumesavetest.qb")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(QBFormatTest, testSaveSingleVoxel) {
	QBFormat f;
	Region region(glm::ivec3(0), glm::ivec3(0));
	RawVolume original(region);
	original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1));
	ASSERT_TRUE(f.save(&original, open("qubicle-singlevoxelsavetest.qb", io::FileMode::Write)));
	f = QBFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("qubicle-singlevoxelsavetest.qb")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(QBFormatTest, testLoadSave) {
	QBFormat f;
	std::unique_ptr<RawVolume> original(load("qubicle.qb", f));
	ASSERT_NE(nullptr, original);
	ASSERT_TRUE(f.save(original.get(), open("qubicle-savetest.qb", io::FileMode::Write)));
	ASSERT_TRUE(open("qubicle-savetest.qb")->length() > 177);
	f = QBFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("qubicle-savetest.qb")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(*original, *loaded);
}
}
