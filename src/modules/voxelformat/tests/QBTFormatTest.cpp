/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QBTFormat.h"

namespace voxel {

class QBTFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBTFormatTest, testLoad) {
	QBTFormat f;
	std::unique_ptr<RawVolume> volume(load("qubicle.qbt", f));
	ASSERT_NE(nullptr, volume) << "Could not load qbt file";
}

TEST_F(QBTFormatTest, testSaveSmallVoxel) {
	QBTFormat f;
	Region region(glm::ivec3(0), glm::ivec3(1));
	RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	ASSERT_TRUE(original.setVoxel(1, 1, 1, createVoxel(VoxelType::Generic, 245)));
	ASSERT_TRUE(original.setVoxel(0, 1, 1, createVoxel(VoxelType::Generic, 127)));
	ASSERT_TRUE(original.setVoxel(0, 1, 0, createVoxel(VoxelType::Generic, 200)));
	ASSERT_TRUE(f.save(&original, open("qubicle-smallvolumesavetest.qbt", io::FileMode::Write)));
	f = QBTFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("qubicle-smallvolumesavetest.qbt")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(QBTFormatTest, testSaveSingleVoxel) {
	QBTFormat f;
	Region region(glm::ivec3(0), glm::ivec3(0));
	RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	ASSERT_TRUE(f.save(&original, open("qubicle-singlevoxelsavetest.qbt", io::FileMode::Write)));
	f = QBTFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("qubicle-singlevoxelsavetest.qbt")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(QBTFormatTest, testSave) {
	QBTFormat f;
	std::unique_ptr<RawVolume> original(load("qubicle.qbt", f));
	ASSERT_NE(nullptr, original);
	ASSERT_TRUE(f.save(original.get(), open("qubicle-savetest.qbt", io::FileMode::Write)));
	ASSERT_TRUE(open("qubicle-savetest.qbt")->length() > 200);
	f = QBTFormat();
	std::unique_ptr<RawVolume> loaded(f.load(open("qubicle-savetest.qbt")));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(*original, *loaded);
}

}
