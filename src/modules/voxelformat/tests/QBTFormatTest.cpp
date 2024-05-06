/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "voxelformat/private/qubicle/QBTFormat.h"

namespace voxelformat {

class QBTFormatTest: public AbstractFormatTest {
};

TEST_F(QBTFormatTest, testLoad) {
	testLoad("qubicle.qbt", 17);
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

TEST_F(QBTFormatTest, testSaveMultipleModels) {
	QBTFormat f;
	testSaveMultipleModels("qubicle-multiplemodelsavetest.qbt", &f);
}

TEST_F(QBTFormatTest, testSave) {
	QBTFormat f;
	testConvert("qubicle.qbt", f, "qubicle-savetest.qbt", f);
}

TEST_F(QBTFormatTest, testResaveMultipleModels) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "qubicle.qbt", 17);
	helper_saveSceneGraph(sceneGraph, "qubicle-savetest.qbt");
	sceneGraph.clear();
	testLoad(sceneGraph, "qubicle-savetest.qbt", 17);
}

}
