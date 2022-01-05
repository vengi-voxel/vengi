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

TEST_F(QBTFormatTest, testSaveSingleVoxel) {
	QBTFormat f;
	Region region(glm::ivec3(0), glm::ivec3(0));
	RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(VoxelType::Generic, 1)));
	const io::FilePtr &file = open("qubicle-singlevoxelsavetest.qbt", io::FileMode::Write);
	io::FileStream stream(file.get());
	ASSERT_TRUE(f.save(&original, file->name(), stream));
	f = QBTFormat();
	std::unique_ptr<RawVolume> loaded(load("qubicle-singlevoxelsavetest.qbt", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(original, *loaded);
}

TEST_F(QBTFormatTest, testSaveSmallVoxel) {
	QBTFormat f;
	testSaveLoadVoxel("qubicle-smallvolumesavetest.qbt", &f);
}

TEST_F(QBTFormatTest, testSaveMultipleLayers) {
	QBTFormat f;
	testSaveMultipleLayers("qubicle-multiplelayersavetest.qbt", &f);
}

TEST_F(QBTFormatTest, testSave) {
	QBTFormat f;
	std::unique_ptr<RawVolume> original(load("qubicle.qbt", f));
	ASSERT_NE(nullptr, original);
	const io::FilePtr &file = open("qubicle-savetest.qbt", io::FileMode::Write);
	io::FileStream stream(file.get());
	ASSERT_TRUE(f.save(original.get(), file->name(), stream));
	EXPECT_TRUE(open("qubicle-savetest.qbt")->length() > 200);
	f = QBTFormat();
	std::unique_ptr<RawVolume> loaded(load("qubicle-savetest.qbt", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_EQ(*original, *loaded);
}

TEST_F(QBTFormatTest, testResaveMultipleLayers) {
	SceneGraph volumes;
	{
		QBTFormat f;
		io::FilePtr file = open("qubicle.qbt");
		io::FileStream stream(file.get());
		EXPECT_TRUE(f.loadGroups(file->name(), stream, volumes));
		EXPECT_EQ(17u, volumes.size());
	}
	{
		QBTFormat f;
		const io::FilePtr &file = open("qubicle-savetest.qbt", io::FileMode::Write);
		io::FileStream stream(file.get());
		EXPECT_TRUE(f.saveGroups(volumes, file->name(), stream));
		EXPECT_EQ(17u, volumes.size());
	}
	volumes.clear();
	{
		QBTFormat f;
		io::FilePtr file = open("qubicle-savetest.qbt");
		io::FileStream stream(file.get());
		EXPECT_TRUE(f.loadGroups(file->name(), stream, volumes));
		EXPECT_EQ(17u, volumes.size());
	}
	volumes.clear();
}

}
