/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "voxel/model/QBFormat.h"

namespace voxel {

class QBFormatTest: public AbstractVoxFormatTest {
};

TEST_F(QBFormatTest, testLoad) {
	QBFormat f;
	RawVolume* volume = load("qubicle.qb", f);
	// feets
	EXPECT_NE(Empty, volume->getVoxel(18, 0, 1));
	EXPECT_NE(Empty, volume->getVoxel(18, 0, 2));
	EXPECT_NE(Empty, volume->getVoxel(18, 0, 3));
	EXPECT_EQ(Empty, volume->getVoxel(18, 0, 4));
	EXPECT_NE(Empty, volume->getVoxel(22, 0, 1));
	EXPECT_NE(Empty, volume->getVoxel(22, 0, 2));
	EXPECT_NE(Empty, volume->getVoxel(22, 0, 3));
	EXPECT_EQ(Empty, volume->getVoxel(22, 0, 4));

	// legs
	EXPECT_NE(Empty, volume->getVoxel(18, 1, 3));
	EXPECT_NE(Empty, volume->getVoxel(18, 2, 3));
	EXPECT_NE(Empty, volume->getVoxel(18, 3, 3));
	EXPECT_EQ(Empty, volume->getVoxel(18, 4, 3));
	EXPECT_NE(Empty, volume->getVoxel(22, 1, 3));
	EXPECT_NE(Empty, volume->getVoxel(22, 2, 3));
	EXPECT_NE(Empty, volume->getVoxel(22, 3, 3));
	EXPECT_EQ(Empty, volume->getVoxel(22, 4, 3));

	ASSERT_NE(nullptr, volume) << "Could not load qb file";
	delete volume;
}

TEST_F(QBFormatTest, testSave) {
	QBFormat f;
	RawVolume* volume = load("qubicle.qb", f);
	ASSERT_NE(nullptr, volume);
	ASSERT_TRUE(f.save(volume, open("qubicle-savetest.qb", io::FileMode::Write)));
	ASSERT_TRUE(open("qubicle-savetest.qb")->length() > 177);
	delete volume;
}
}
