/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/FileStream.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/VolumeFormat.h"

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
	std::unique_ptr<RawVolume> loaded(load("qubicle-smallvolumesavetest.qbt", f));
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
	std::unique_ptr<RawVolume> loaded(load("qubicle-singlevoxelsavetest.qbt", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(QBTFormatTest, testSaveMultipleLayers) {
	QBTFormat f;
	Region region(glm::ivec3(0), glm::ivec3(0));
	RawVolume layer1(region);
	RawVolume layer2(region);
	RawVolume layer3(region);
	RawVolume layer4(region);
	EXPECT_TRUE(layer1.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(layer2.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(layer3.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	EXPECT_TRUE(layer4.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	VoxelVolumes volumes;
	volumes.push_back(VoxelVolume(&layer1));
	volumes.push_back(VoxelVolume(&layer2));
	volumes.push_back(VoxelVolume(&layer3));
	volumes.push_back(VoxelVolume(&layer4));
	EXPECT_TRUE(f.saveGroups(volumes, open("qubicle-multiplelayersavetest.qbt", io::FileMode::Write)));
	f = QBTFormat();
	VoxelVolumes volumesLoad;
	const io::FilePtr &file = open("qubicle-multiplelayersavetest.qbt");
	io::FileStream stream(file.get());
	EXPECT_TRUE(f.loadGroups(file->fileName(), stream, volumesLoad));
	EXPECT_EQ(volumesLoad.size(), volumes.size());
	voxelformat::clearVolumes(volumesLoad);
}

TEST_F(QBTFormatTest, testSave) {
	QBTFormat f;
	std::unique_ptr<RawVolume> original(load("qubicle.qbt", f));
	ASSERT_NE(nullptr, original);
	EXPECT_TRUE(f.save(original.get(), open("qubicle-savetest.qbt", io::FileMode::Write)));
	EXPECT_TRUE(open("qubicle-savetest.qbt")->length() > 200);
	f = QBTFormat();
	std::unique_ptr<RawVolume> loaded(load("qubicle-savetest.qbt", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(*original, *loaded);
}

TEST_F(QBTFormatTest, testResaveMultipleLayers) {
	VoxelVolumes volumes;
	{
		QBTFormat f;
		io::FilePtr file = open("qubicle.qbt");
		io::FileStream stream(file.get());
		EXPECT_TRUE(f.loadGroups(file->fileName(), stream, volumes));
		EXPECT_EQ(17u, volumes.size());
	}
	{
		QBTFormat f;
		EXPECT_TRUE(f.saveGroups(volumes, open("qubicle-savetest.qbt", io::FileMode::Write)));
		EXPECT_EQ(17u, volumes.size());
	}
		voxelformat::clearVolumes(volumes);
	{
		QBTFormat f;
		io::FilePtr file = open("qubicle-savetest.qbt");
		io::FileStream stream(file.get());
		EXPECT_TRUE(f.loadGroups(file->fileName(), stream, volumes));
		EXPECT_EQ(17u, volumes.size());
	}
	voxelformat::clearVolumes(volumes);
}

}
