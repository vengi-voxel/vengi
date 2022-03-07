/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/VolumeFormat.h"

namespace voxel {

class QBFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBFormatTest, testLoad) {
	QBFormat f;
	std::unique_ptr<RawVolume> volume(load("qubicle.qb", f));

	VolumePrintThreshold = 40;
	ASSERT_NE(nullptr, volume) << "Could not load qb file";

	// feet
	//EXPECT_NE(Empty, volume->voxel(-3, 0, -1)) << *volume;
	//EXPECT_NE(Empty, volume->voxel(-6, 0, -1)) << *volume;
	EXPECT_EQ(Empty, volume->voxel(-3, 5, -1)) << *volume;
	EXPECT_EQ(Empty, volume->voxel(-6, 5, -1)) << *volume;
	//EXPECT_NE(Empty, volume->voxel(-3, 1,  2)) << *volume;
	//EXPECT_NE(Empty, volume->voxel(-6, 1,  2)) << *volume;
	EXPECT_EQ(Empty, volume->voxel(-3, 4, -1)) << *volume;
	EXPECT_EQ(Empty, volume->voxel(-6, 4, -1)) << *volume;
}

TEST_F(QBFormatTest, testLoadRGB) {
	QBFormat f;
	std::unique_ptr<RawVolume> volume(load("rgb.qb", f));
	ASSERT_NE(nullptr, volume) << "Could not load qb file";
	testRGB(volume.get());
}

TEST_F(QBFormatTest, testSaveSingleVoxel) {
	QBFormat f;
	Region region(glm::ivec3(0), glm::ivec3(0));
	RawVolume original(region);
	original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1));
	const io::FilePtr &file = open("qubicle-singlevoxelsavetest.qb", io::FileMode::SysWrite);
	io::FileStream stream(file);
	ASSERT_TRUE(f.save(&original, file->name(), stream));
	f = QBFormat();
	std::unique_ptr<RawVolume> loaded(load("qubicle-singlevoxelsavetest.qb", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(QBFormatTest, testSaveSmallVoxel) {
	QBFormat f;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qb", &f);
}

TEST_F(QBFormatTest, testSaveMultipleLayers) {
	QBFormat f;
	testSaveMultipleLayers("qubicle-multiplelayersavetest.qb", &f);
}

TEST_F(QBFormatTest, testLoadSave) {
	QBFormat f;
	std::unique_ptr<RawVolume> original(load("qubicle.qb", f));
	ASSERT_NE(nullptr, original);
	const io::FilePtr &sfile = open("qubicle-savetest.qb", io::FileMode::SysWrite);
	io::FileStream sstream(sfile);
	ASSERT_TRUE(f.save(original.get(), sfile->name(), sstream));
	ASSERT_TRUE(open("qubicle-savetest.qb")->length() > 177);
	f = QBFormat();
	std::unique_ptr<RawVolume> loaded(load("qubicle-savetest.qb", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(*original, *loaded);
}
}
