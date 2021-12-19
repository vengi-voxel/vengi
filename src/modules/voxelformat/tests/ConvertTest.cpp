/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "voxelformat/BinVoxFormat.h"
#include "voxelformat/CubFormat.h"
#include "voxelformat/GoxFormat.h"
#include "voxelformat/KV6Format.h"
#include "voxelformat/KVXFormat.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/SproxelFormat.h"
#include "voxelformat/VXLFormat.h"
#include "voxelformat/VXMFormat.h"
#include "voxelformat/VXRFormat.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/VoxFormat.h"

namespace voxel {

class ConvertTest: public AbstractVoxFormatTest {
};

TEST_F(ConvertTest, testQbToVox) {
	QBFormat src;
	VoxFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.vox", target, true);
}

TEST_F(ConvertTest, testVoxToQb) {
	VoxFormat src;
	QBFormat target;
	testLoadSaveAndLoad("rgb.vox", src, "test.qb", target, true);
}

// TODO: broken
TEST_F(ConvertTest, DISABLED_testQbToBinvox) {
	QBFormat src;
	BinVoxFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.binvox", target, false);
}

TEST_F(ConvertTest, testBinvoxToQb) {
	BinVoxFormat src;
	QBFormat target;
	testLoadSaveAndLoad("test.binvox", src, "test.qb", target, true);
}

TEST_F(ConvertTest, testQbToVXL) {
	QBFormat src;
	VXLFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.vxl", target, true);
}

TEST_F(ConvertTest, testVXLToQb) {
	VXLFormat src;
	QBFormat target;
	testLoadSaveAndLoad("rgb.vxl", src, "test.qb", target, true);
}

TEST_F(ConvertTest, testQbToQbt) {
	QBFormat src;
	QBTFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.qbt", target, true);
}

TEST_F(ConvertTest, testQbtToQb) {
	QBTFormat src;
	QBFormat target;
	testLoadSaveAndLoad("qubicle.qbt", src, "test.qb", target, true);
}

TEST_F(ConvertTest, testQbToSproxel) {
	QBFormat src;
	SproxelFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.csv", target, true);
}

TEST_F(ConvertTest, testSproxelToQb) {
	SproxelFormat src;
	QBFormat target;
	testLoadSaveAndLoad("rgb.csv", src, "test.qb", target, true);
}

TEST_F(ConvertTest, testQbToVXM) {
	QBFormat src;
	VXMFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.vxm", target, true);
}

TEST_F(ConvertTest, testVXMToQb) {
	VXMFormat src;
	QBFormat target;
	testLoadSaveAndLoad("test.vxm", src, "test.qb", target, true);
}

TEST_F(ConvertTest, testQbToCub) {
	QBFormat src;
	CubFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.cub", target, true);
}

TEST_F(ConvertTest, testCubToQb) {
	CubFormat src;
	QBFormat target;
	testLoadSaveAndLoad("rgb.cub", src, "test.qb", target, true);
}

TEST_F(ConvertTest, testKVXToQb) {
	KVXFormat src;
	QBFormat target;
	testLoadSaveAndLoad("test.kvx", src, "test.qb", target, true);
}

TEST_F(ConvertTest, testKV6ToQb) {
	KV6Format src;
	QBFormat target;
	testLoadSaveAndLoad("test.kv6", src, "test.qb", target, true);
}

TEST_F(ConvertTest, testQbToVXR) {
	QBFormat src;
	VXRFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.vxr", target, true);
}

// a source file is missing here
TEST_F(ConvertTest, DISABLED_testVXRToQb) {
	VXRFormat src;
	QBFormat target;
	testLoadSaveAndLoad("test.vxr", src, "test.qb", target, true);
}

// TODO: broken
TEST_F(ConvertTest, DISABLED_testQbToGox) {
	QBFormat src;
	GoxFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.gox", target, true);
}

TEST_F(ConvertTest, testGoxToQb) {
	GoxFormat src;
	QBFormat target;
	testLoadSaveAndLoad("test.gox", src, "test.qb", target, true);
}

}
