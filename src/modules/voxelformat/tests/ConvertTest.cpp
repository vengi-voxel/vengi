/**
 * @file
 */

#include "AbstractVoxFormatTest.h"
#include "io/BufferedReadWriteStream.h"
#include "io/FileStream.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/binvox/BinVoxFormat.h"
#include "voxelformat/private/commandconquer/VXLFormat.h"
#include "voxelformat/private/cubeworld/CubFormat.h"
#include "voxelformat/private/goxel/GoxFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/magicavoxel/XRawFormat.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
#include "voxelformat/private/mesh/OBJFormat.h"
#include "voxelformat/private/mesh/STLFormat.h"
#include "voxelformat/private/qubicle/QBCLFormat.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "voxelformat/private/qubicle/QBTFormat.h"
#include "voxelformat/private/sandbox/VXMFormat.h"
#include "voxelformat/private/sandbox/VXRFormat.h"
#include "voxelformat/private/slab6/KV6Format.h"
#include "voxelformat/private/slab6/KVXFormat.h"
#include "voxelformat/private/slab6/SLAB6VoxFormat.h"
#include "voxelformat/private/sproxel/SproxelFormat.h"
#include "voxelformat/tests/TestHelper.h"

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
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.vox", target, flags, 0.004f);
}

TEST_F(ConvertTest, testVoxToQb) {
	VoxFormat src;
	QBFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "convert-robo.qb", target, flags);
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
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.obj", target, flags, 0.014f);
}

TEST_F(ConvertTest, testBinvoxToQb) {
	BinVoxFormat src;
	QBFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testLoadSaveAndLoadSceneGraph("test.binvox", src, "convert-test.qb", target, flags);
}

TEST_F(ConvertTest, testVXLToQb) {
	VXLFormat src;
	QBFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testLoadSaveAndLoadSceneGraph("rgb.vxl", src, "convert-rgb.qb", target, flags);
}

TEST_F(ConvertTest, testQbToQbt) {
	QBFormat src;
	QBTFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.qbt", target, flags);
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
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testLoadSaveAndLoadSceneGraph("rgb.csv", src, "convert-rgb.qb", target, flags);
}

TEST_F(ConvertTest, testQbToQb) {
	QBFormat src;
	QBFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.qb", target);
}

TEST_F(ConvertTest, testQbToCub) {
	QBFormat src;
	CubFormat target;
	// order of colors in palette differs
	// TODO: this could be an own flag to still validate the colors are the same, without taking care about the order)
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette);
	testLoadSaveAndLoad("chr_knight.qb", src, "convert-chr_knight.cub", target, flags);
}

TEST_F(ConvertTest, testCubToQb) {
	CubFormat src;
	QBFormat target;
	// qb doesn't build a palette yet
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("rgb.cub", src, "convert-rgb.qb", target, flags);
}

TEST_F(ConvertTest, testGoxToQb) {
	GoxFormat src;
	QBFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testLoadSaveAndLoadSceneGraph("test.gox", src, "convert-test.qb", target, flags);
}

TEST_F(ConvertTest, testQBCLToQb) {
	QBCLFormat src;
	QBFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("qubicle.qbcl", src, "convert-qubicle.qb", target, flags);
}

TEST_F(ConvertTest, testQbtToQb) {
	QBTFormat src;
	QBFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("qubicle.qbt", src, "convert-qubicle.qb", target, flags);
}

TEST_F(ConvertTest, testKV6ToQb) {
	KV6Format src;
	QBFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors & ~(voxel::ValidateFlags::Pivot);
	testLoadSaveAndLoadSceneGraph("test.kv6", src, "convert-test.qb", target, flags);
}

TEST_F(ConvertTest, testQbToVXR) {
	QBFormat src;
	VXRFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("robo.qb", src, "convert-robo.vxr", target, flags);
}

TEST_F(ConvertTest, testQbToQBCL) {
	QBFormat src;
	QBCLFormat target;
	testLoadSaveAndLoadSceneGraph("rgb.qb", src, "convert-rgb.qbcl", target);
}

TEST_F(ConvertTest, testQbToVXM) {
	QBFormat src;
	VXMFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	// vxm doesn't store the position - this is handled in vxr/vxa - so it's ok here to skip the translation check
	// qb doesn't store the pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Translation | voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.vxm", target, flags);
}

TEST_F(ConvertTest, testQbToVXL) {
	QBFormat src;
	VXLFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette;
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.vxl", target, flags);
}

TEST_F(ConvertTest, testQBCLToQBCL) {
	QBCLFormat src;
	QBCLFormat target;
	testLoadSaveAndLoadSceneGraph("chr_knight.qbcl", src, "convert-chr_knight.qbcl", target);
}

TEST_F(ConvertTest, testVXMToQb) {
	VXMFormat src;
	QBFormat target;
	// the palette color amount differs, because qubicle is a rgba format and only stores used colors
	// qb doesn't store a pivot
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("test.vxm", src, "convert-test.qb", target, flags);
}

TEST_F(ConvertTest, testVXRToQb) {
	VXRFormat src;
	QBFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	// qb doesn't store a pivot
	// qb doesn't allow animations
	// qb stores translation as integer, vxr as float
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Palette | voxel::ValidateFlags::Animations | voxel::ValidateFlags::Translation);
	testLoadSaveAndLoadSceneGraph("e2de1723/e2de1723.vxr", src, "convert-e2de1723.qb", target, flags);
}

TEST_F(ConvertTest, testKVXToQb) {
	KVXFormat src;
	QBFormat target;
	// qb doesn't store a pivot
	// TODO: this could be an own flag to still validate the colors are the same, without taking care about the order)
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoad("test.kvx", src, "convert-test.qb", target, flags);
}

TEST_F(ConvertTest, testLoadRGBSmallVoxToQb) {
	testRGBSmallSaveLoad("rgb_small.vox", "test.qb");
}

TEST_F(ConvertTest, testLoadRGBSmallVoxToXRaw) {
	testRGBSmallSaveLoad("rgb_small.vox", "test.xraw");
}

TEST_F(ConvertTest, testLoadRGBSmallQbToVox) {
	testRGBSmallSaveLoad("rgb_small.qb", "test.vox");
}

TEST_F(ConvertTest, testLoadRGBSmallVoxToQbcl) {
	testRGBSmallSaveLoad("rgb_small.vox", "test.qbcl");
}

TEST_F(ConvertTest, testLoadRGBSmallQbclToVox) {
	testRGBSmallSaveLoad("rgb_small.qbcl", "test.vox");
}

TEST_F(ConvertTest, testVXLToVXR) {
	VXLFormat src;
	VXRFormat target;
	// the palette of vxm contains one transparent entry that is used to indicate empty voxels - thus the palette has one entry less
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("cc.vxl", src, "convert-cc.vxr", target, flags);
}

TEST_F(ConvertTest, testKV6ToKV6) {
	KV6Format src;
	KV6Format target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testLoadSaveAndLoad("test.kv6", src, "convert-test.kv6", target, flags);
}

TEST_F(ConvertTest, testKV6ToKV62) {
	KV6Format src;
	KV6Format target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testLoadSaveAndLoad("test2.kv6", src, "convert-test2.kv6", target, flags);
}

TEST_F(ConvertTest, testQBToSLAB6Vox) {
	SLAB6VoxFormat src;
	SLAB6VoxFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All;
	testLoadSaveAndLoad("slab6_vox_test.vox", src, "convert-slab6_vox_test.vox", target, flags);
}

// TODO: pivot broken
// TODO: broken keyframes
// TODO: broken voxels
TEST_F(ConvertTest, DISABLED_testGLTFToGLTF) {
	GLTFFormat src;
	GLTFFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Pivot);
	testLoadSaveAndLoadSceneGraph("glTF/BoxAnimated.glb", src, "convert-BoxAnimated2.glb", target, flags);
}

// TODO: pivot broken
// TODO: translation broken
TEST_F(ConvertTest, testVoxToVXR) {
	VoxFormat src;
	VXMFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Translation | voxel::ValidateFlags::Pivot);
	testLoadSaveAndLoadSceneGraph("robo.vox", src, "convert-robo.vxr", target, flags);
}

// TODO: translation broken
TEST_F(ConvertTest, testQbToGox) {
	QBFormat src;
	GoxFormat target;
	// qubicle doesn't store all colors in the palette - but only the used colors - that's why the amount might differ
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~(voxel::ValidateFlags::Translation | voxel::ValidateFlags::Palette);
	testLoadSaveAndLoadSceneGraph("chr_knight.qb", src, "convert-chr_knight.gox", target, flags);
}

TEST_F(ConvertTest, testQBToKVX) {
	QBFormat src;
	KVXFormat target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors | voxel::ValidateFlags::IgnoreHollow;
	testLoadSaveAndLoad("kvx_save.qb", src, "convert-kvx_save.kvx", target, flags);
}

TEST_F(ConvertTest, testQBChrKnightToKVX) {
	QBFormat src;
	KVXFormat target;
	// KVX has all colors in the palette set - and thus the color amount doesn't match
	const voxel::ValidateFlags flags = voxel::ValidateFlags::All & ~voxel::ValidateFlags::Palette | voxel::ValidateFlags::IgnoreHollow;
	testLoadSaveAndLoad("chr_knight.qb", src, "convert-chr_knight.kvx", target, flags);
}

TEST_F(ConvertTest, testQBToKV6) {
	QBFormat src;
	KV6Format target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors | voxel::ValidateFlags::IgnoreHollow;
	testLoadSaveAndLoad("kvx_save.qb", src, "convert-kvx_save.kv6", target, flags);
}

TEST_F(ConvertTest, testKVXToKVX) {
	KVXFormat src;
	KVXFormat target;
	const voxel::ValidateFlags flags = (voxel::ValidateFlags::All | voxel::ValidateFlags::IgnoreHollow) & ~(voxel::ValidateFlags::Palette);
	testLoadSaveAndLoad("test.kvx", src, "convert-test.kvx", target, flags);
}

TEST_F(ConvertTest, DISABLED_testQBChrKnightToKV6) {
	QBFormat src;
	KV6Format target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors | voxel::ValidateFlags::IgnoreHollow;
	testLoadSaveAndLoad("chr_knight.qb", src, "convert-chr_knight.kv6", target, flags);
}

TEST_F(ConvertTest, DISABLED_testKVXToKV6) {
	KVXFormat src;
	KV6Format target;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors | voxel::ValidateFlags::IgnoreHollow;
	testLoadSaveAndLoad("test.kvx", src, "convert-test.kv6", target, flags, 0.0022f);
}

}
