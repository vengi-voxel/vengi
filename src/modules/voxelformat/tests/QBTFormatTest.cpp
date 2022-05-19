/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/FileStream.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/VolumeFormat.h"

namespace voxelformat {

class QBTFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBTFormatTest, testLoad) {
	QBTFormat f;
	std::unique_ptr<voxel::RawVolume> volume(load("qubicle.qbt", f));
	ASSERT_NE(nullptr, volume) << "Could not load qbt file";
}

TEST_F(QBTFormatTest, testSaveSingleVoxel) {
	QBTFormat f;
	voxel::Region region(glm::ivec3(0), glm::ivec3(0));
	voxel::RawVolume original(region);
	ASSERT_TRUE(original.setVoxel(0, 0, 0, createVoxel(voxel::VoxelType::Generic, 1)));
	const io::FilePtr &file = open("qubicle-singlevoxelsavetest.qbt", io::FileMode::SysWrite);
	io::FileStream stream(file);
	ASSERT_TRUE(f.save(&original, file->name(), stream));
	f = QBTFormat();
	std::unique_ptr<voxel::RawVolume> loaded(load("qubicle-singlevoxelsavetest.qbt", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_TRUE(volumeComparator(original, *loaded, true, true)) << "Volumes differ: " << original << *loaded;
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
	std::unique_ptr<voxel::RawVolume> original(load("qubicle.qbt", f));
	ASSERT_NE(nullptr, original);
	const io::FilePtr &file = open("qubicle-savetest.qbt", io::FileMode::SysWrite);
	io::FileStream stream(file);
	ASSERT_TRUE(f.save(original.get(), file->name(), stream));
	EXPECT_TRUE(open("qubicle-savetest.qbt")->length() > 200);
	f = QBTFormat();
	std::unique_ptr<voxel::RawVolume> loaded(load("qubicle-savetest.qbt", f));
	ASSERT_NE(nullptr, loaded);
	EXPECT_TRUE(volumeComparator(*original, *loaded, true, true)) << "Volumes differ: " << *original << *loaded;
}

TEST_F(QBTFormatTest, testResaveMultipleLayers) {
	SceneGraph sceneGraph;
	{
		QBTFormat f;
		io::FilePtr file = open("qubicle.qbt");
		io::FileStream stream(file);
		EXPECT_TRUE(f.loadGroups(file->name(), stream, sceneGraph));
		EXPECT_EQ(17u, sceneGraph.size());
	}
	{
		QBTFormat f;
		const io::FilePtr &file = open("qubicle-savetest.qbt", io::FileMode::SysWrite);
		io::FileStream stream(file);
		EXPECT_TRUE(f.saveGroups(sceneGraph, file->name(), stream));
		EXPECT_EQ(17u, sceneGraph.size());
	}
	sceneGraph.clear();
	{
		QBTFormat f;
		io::FilePtr file = open("qubicle-savetest.qbt");
		io::FileStream stream(file);
		EXPECT_TRUE(f.loadGroups(file->name(), stream, sceneGraph));
		EXPECT_EQ(17u, sceneGraph.size());
	}
}

}
