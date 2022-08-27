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

namespace voxelformat {

class ConvertTest: public AbstractVoxFormatTest {
};

TEST_F(ConvertTest, testVoxToVXMPalette) {
	VoxFormat src;
	VXMFormat target;
	testFirstAndLastPaletteIndexConversion(src, "palette-check.vxm", target, false, true);
}

TEST_F(ConvertTest, testVoxToVXM) {
	VoxFormat src;
	VXMFormat target;
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "robo.vxm", target, true, false, false);
}

// TODO: pivot broken
// TODO: transform broken
TEST_F(ConvertTest, testVoxToVXR) {
	VoxFormat src;
	VXMFormat target;
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "robo.vxr", target, true, false, false);
}

TEST_F(ConvertTest, testQbToVox) {
	QBFormat src;
	VoxFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "chr_knight2.vox", target, true, true, true);
}

TEST_F(ConvertTest, testVoxToQb) {
	VoxFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "robo2.qb", target, true, true, true);
}

TEST_F(ConvertTest, testVoxToVox) {
	VoxFormat src;
	VoxFormat target;
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "robo2.vox", target, true, true, true);
}

TEST_F(ConvertTest, testQbToBinvox) {
	QBFormat src;
	BinVoxFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.binvox", target, false, false);
}

TEST_F(ConvertTest, testBinvoxToQb) {
	BinVoxFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("test.binvox", src, "test.qb", target, true, true, true);
}

// TODO: transform broken
TEST_F(ConvertTest, testQbToVXL) {
	QBFormat src;
	VXLFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "chr_knight.vxl", target, true, false, false);
}

TEST_F(ConvertTest, testVXLToQb) {
	VXLFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("rgb.vxl", src, "test.qb", target, true, true, true);
}

TEST_F(ConvertTest, testQbToQbt) {
	QBFormat src;
	QBTFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "chr_knight.qbt", target, true, true, true);
}

// TODO: pivot broken
TEST_F(ConvertTest, testQbtToQb) {
	QBTFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("qubicle.qbt", src, "test.qb", target, true, true, false);
}

TEST_F(ConvertTest, testQbToSproxel) {
	QBFormat src;
	SproxelFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.csv", target, true, false);
}

TEST_F(ConvertTest, testSproxelToQb) {
	SproxelFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("rgb.csv", src, "test.qb", target, true, true, true);
}

TEST_F(ConvertTest, testQbToQb) {
	QBFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "chr_knight2.qb", target, true, true, true);
}

// TODO: transform broken
TEST_F(ConvertTest, testQbToVXM) {
	QBFormat src;
	VXMFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "chr_knight.vxm", target, true, false, false);
}

// TODO: pivot broken
TEST_F(ConvertTest, testVXMToQb) {
	VXMFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("test.vxm", src, "test.qb", target, false, true, false);
}

TEST_F(ConvertTest, testQbToCub) {
	QBFormat src;
	CubFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "chr_knight.cub", target, true, false, true);
}

TEST_F(ConvertTest, testCubToQb) {
	CubFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("rgb.cub", src, "test.qb", target, true, true, true);
}

// TODO: pivot broken
TEST_F(ConvertTest, testKVXToQb) {
	KVXFormat src;
	QBFormat target;
	testLoadSaveAndLoad("test.kvx", src, "test.qb", target, true, true, 0.012f);
}

// TODO: pivot broken
TEST_F(ConvertTest, testKV6ToQb) {
	KV6Format src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("test.kv6", src, "test.qb", target, true, true, false);
}

// TODO: pivot broken
TEST_F(ConvertTest, testQbToVXR) {
	QBFormat src;
	VXRFormat target;
	testLoadSaveAndLoadSceneGraph("robo.qb", src, "robo.vxr", target, true, false, false);
}

// TODO: colors broken
// TODO: transform broken
TEST_F(ConvertTest, testVXRToQb) {
	VXRFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("e2de1723/e2de1723.vxr", src, "test.qb", target, false, false, false);
}

// TODO: colors broken
// TODO: transform broken
TEST_F(ConvertTest, DISABLED_testQbToGox) {
	QBFormat src;
	GoxFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "chr_knight.gox", target, false, false, false);
}

// TODO: transform broken
TEST_F(ConvertTest, testGoxToQb) {
	GoxFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("test.gox", src, "test.qb", target, true, true, false);
}

}
