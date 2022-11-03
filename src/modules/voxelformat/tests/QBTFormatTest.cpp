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
	canLoad("qubicle.qbt", 17);
}

TEST_F(QBTFormatTest, testLoadRGBSmall) {
	testRGBSmall("rgb_small.qbt");
}

TEST_F(QBTFormatTest, testLoadRGBSmallSaveLoad) {
	testRGBSmallSaveLoad("rgb_small.qbt");
}

TEST_F(QBTFormatTest, testSaveSingleVoxel) {
	QBTFormat f;
	testSaveSingleVoxel("qubicle-singlevoxelsavetest.qb", &f);
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
	testLoadSaveAndLoad("qubicle.qbt", f, "qubicle-savetest.qbt", f);
}

TEST_F(QBTFormatTest, testResaveMultipleLayers) {
	SceneGraph sceneGraph;
	{
		QBTFormat f;
		io::FilePtr file = open("qubicle.qbt");
		io::FileStream stream(file);
		EXPECT_TRUE(f.load(file->name(), stream, sceneGraph));
		EXPECT_EQ(17u, sceneGraph.size());
	}
	{
		QBTFormat f;
		const io::FilePtr &file = open("qubicle-savetest.qbt", io::FileMode::SysWrite);
		io::FileStream stream(file);
		EXPECT_TRUE(f.save(sceneGraph, file->name(), stream, testThumbnailCreator));
		EXPECT_EQ(17u, sceneGraph.size());
	}
	sceneGraph.clear();
	{
		QBTFormat f;
		io::FilePtr file = open("qubicle-savetest.qbt");
		io::FileStream stream(file);
		EXPECT_TRUE(f.load(file->name(), stream, sceneGraph));
		EXPECT_EQ(17u, sceneGraph.size());
	}
}

}
