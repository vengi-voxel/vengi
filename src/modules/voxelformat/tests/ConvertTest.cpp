/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "voxel/tests/TestHelper.h"
#include "voxelformat/BinVoxFormat.h"
#include "voxelformat/CubFormat.h"
#include "voxelformat/GLTFFormat.h"
#include "voxelformat/GoxFormat.h"
#include "voxelformat/KV6Format.h"
#include "voxelformat/KVXFormat.h"
#include "voxelformat/OBJFormat.h"
#include "voxelformat/QBCLFormat.h"
#include "voxelformat/QBFormat.h"
#include "voxelformat/QBTFormat.h"
#include "voxelformat/STLFormat.h"
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
	testFirstAndLastPaletteIndexConversion(src, "palette-check.vxm", target, voxel::ValidateFlags::Region);
}

TEST_F(ConvertTest, testVoxToVXM) {
	VoxFormat src;
	VXMFormat target;
	// vxm can't store transforms - only the voxel data.
	const voxel::ValidateFlags flags = voxel::ValidateFlags::Color;
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "convert-robo.vxm", target, flags);
}

TEST_F(ConvertTest, testQbToVox) {
	QBFormat src;
	VoxFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.vox", target);
}

TEST_F(ConvertTest, testVoxToQb) {
	VoxFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "convert-robo.qb", target);
}

TEST_F(ConvertTest, testVoxToVox) {
	VoxFormat src;
	VoxFormat target;
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "convert-robo.vox", target);
}

TEST_F(ConvertTest, testQbToBinvox) {
	QBFormat src;
	BinVoxFormat target;
	// binvox doesn't have colors and is a single volume format (no need to check transforms)
	const voxel::ValidateFlags flags = voxel::ValidateFlags::None;
	testLoadSaveAndLoad("chr_knight.qb", src, "convert-chr_knight.binvox", target, flags);
}

TEST_F(ConvertTest, testQbToSTL) {
	QBFormat src;
	STLFormat target;
	// stl doesn't have colors and is a single volume format (no need to check transforms)
	const voxel::ValidateFlags flags = voxel::ValidateFlags::None;
	testLoadSaveAndLoad("chr_knight.qb", src, "convert-chr_knight.stl", target, flags);
}

TEST_F(ConvertTest, testQbToOBJ) {
	QBFormat src;
	OBJFormat target;
	// the palette size is reduced here to the real amount of used colors
	const voxel::ValidateFlags flags = (voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette) | voxel::ValidateFlags::IgnoreHollow;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.obj", target, flags);
}

TEST_F(ConvertTest, DISABLED_testGLTFToGLTF) {
	GLTFFormat src;
	GLTFFormat target;
	testLoadSaveAndLoadSceneGraph("glTF/BoxAnimated.glb", src, "convert-BoxAnimated2.glb", target);
}

TEST_F(ConvertTest, testBinvoxToQb) {
	BinVoxFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("test.binvox", src, "convert-test.qb", target);
}

TEST_F(ConvertTest, testVXLToQb) {
	VXLFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("rgb.vxl", src, "convert-rgb.qb", target);
}

TEST_F(ConvertTest, testQbToQbt) {
	QBFormat src;
	QBTFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.qbt", target);
}

TEST_F(ConvertTest, testQbToSproxel) {
	QBFormat src;
	SproxelFormat target;
	// sproxel csv can't store transforms - only the voxel data.
	const voxel::ValidateFlags flags = voxel::ValidateFlags::Color;
	testLoadSaveAndLoad("chr_knight.qb", src, "convert-chr_knight.csv", target, flags);
}

TEST_F(ConvertTest, testSproxelToQb) {
	SproxelFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("rgb.csv", src, "convert-rgb.qb", target);
}

TEST_F(ConvertTest, testQbToQb) {
	QBFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.qb", target);
}

TEST_F(ConvertTest, testQbToCub) {
	QBFormat src;
	CubFormat target;
	testLoadSaveAndLoad("chr_knight.qb", src, "convert-chr_knight.cub", target);
}

TEST_F(ConvertTest, testCubToQb) {
	CubFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("rgb.cub", src, "convert-rgb.qb", target);
}

TEST_F(ConvertTest, testGoxToQb) {
	GoxFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("test.gox", src, "convert-test.qb", target);
}

TEST_F(ConvertTest, testQBCLToQb) {
	QBCLFormat src;
	QBFormat target;
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot);
	testLoadSaveAndLoadSceneGraph("qubicle.qbcl", src, "convert-qubicle.qb", target, flags);
}

TEST_F(ConvertTest, testQbtToQb) {
	QBTFormat src;
	QBFormat target;
	// qb doesn't store a pivot
	voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot);
	testLoadSaveAndLoadSceneGraph("qubicle.qbt", src, "convert-qubicle.qb", target, flags);
}

TEST_F(ConvertTest, testKVXToQb) {
	KVXFormat src;
	QBFormat target;
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot);
	const float maxDelta = 0.012f;
	testLoadSaveAndLoad("test.kvx", src, "convert-test.qb", target, flags, maxDelta);
}

TEST_F(ConvertTest, testKV6ToQb) {
	KV6Format src;
	QBFormat target;
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot);
	testLoadSaveAndLoadSceneGraph("test.kv6", src, "convert-test.qb", target, flags);
}

TEST_F(ConvertTest, testQbToVXR) {
	QBFormat src;
	VXRFormat target;
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot);
	testLoadSaveAndLoadSceneGraph("robo.qb", src, "convert-robo.vxr", target, flags);
}

// TODO: pivot broken
// TODO: transform broken
TEST_F(ConvertTest, testQbToQBCL) {
	QBFormat src;
	QBCLFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Transform | voxel::ValidateFlags::Pivot);
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.qbcl", target, flags);
}

// TODO: pivot broken
// TODO: transform broken
TEST_F(ConvertTest, testVoxToVXR) {
	VoxFormat src;
	VXMFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Transform | voxel::ValidateFlags::Pivot);
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "convert-robo.vxr", target, flags);
}

// TODO: translation broken
TEST_F(ConvertTest, testQbToVXL) {
	QBFormat src;
	VXLFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Transform);
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.vxl", target, flags);
}

// TODO: translation broken
TEST_F(ConvertTest, testVXLToVXR) {
	VXLFormat src;
	QBFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Transform);
	testLoadSaveAndLoadSceneGraph("cc.vxl", src, "convert-cc.vxr", target, flags);
}

// TODO: transform broken
TEST_F(ConvertTest, testQbToVXM) {
	QBFormat src;
	VXMFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Transform);
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.vxm", target, flags);
}

// TODO: colors broken
TEST_F(ConvertTest, testVXMToQb) {
	VXMFormat src;
	QBFormat target;
	// the palette color amount differs, because qubicle is a rgba format and only stores used colors
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Color | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("test.vxm", src, "convert-test.qb", target, flags);
}

// TODO: colors broken
TEST_F(ConvertTest, testVXRToQb) {
	VXRFormat src;
	QBFormat target;
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Color | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("e2de1723/e2de1723.vxr", src, "convert-e2de1723.qb", target, flags);
}

// TODO: transform broken
TEST_F(ConvertTest, testQbToGox) {
	QBFormat src;
	GoxFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Transform);
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.gox", target, flags);
}

}
