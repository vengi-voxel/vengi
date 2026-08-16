/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "io/Archive.h"
#include "io/FormatDescription.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/minecraft/MCRFormat.h"
#include "voxelformat/private/minecraft/NamedBinaryTag.h"

namespace voxelformat {

class MCRFormatTest : public AbstractFormatTest {};

class TestMCRFormat : public MCRFormat {
public:
	using MCRFormat::MAX_SIZE;
	using MCRFormat::MinecraftSectionPalette;
	using MCRFormat::SectionVolumes;
	using MCRFormat::parseBlockStates;
};

TEST_F(MCRFormatTest, testLoad117) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "r.0.-2.mca", 128);
	const scenegraph::SceneGraphNode &node = *sceneGraph.begin(scenegraph::SceneGraphNodeType::Model);
	ASSERT_EQ(node.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::RawVolume *v = node.volume();
	const voxel::Voxel vxls[] = {
		v->voxel(0, 62, -576),
		v->voxel(0, -45, -576),
		v->voxel(0, -45, -566),
		v->voxel(0, -62, -576),
		v->voxel(0, -64, -576)
	};
	const uint8_t clrs[] = {
		22,
		22,
		6,
		118,
		7
	};
	for (int i = 0; i < 5; ++i) {
		const voxel::Voxel expected = voxel::createVoxel(voxel::VoxelType::Generic, clrs[i]);
		EXPECT_TRUE(vxls[i].isSame(expected)) << vxls[i];
	}
	EXPECT_EQ(32512, v->region().voxels());
}

TEST_F(MCRFormatTest, testLoad117SeparateWater) {
	util::ScopedVarChange scoped(cfg::VoxformatMCSeparateWater, "true");
	scenegraph::SceneGraph sceneGraph;
	const io::ArchivePtr &archive = helper_filesystemarchive();
	if (!archive->exists("r.0.-2.mca")) {
		GTEST_SKIP() << "Could not open r.0.-2.mca";
	}
	io::FileDescription fileDesc;
	fileDesc.set("r.0.-2.mca");
	ASSERT_TRUE(voxelformat::loadFormat(fileDesc, archive, sceneGraph, testLoadCtx));

	int waterNodes = 0;
	bool foundWaterVoxel = false;
	bool foundWaterInTerrain = false;
	sceneGraph.visitChildren(sceneGraph.root().id(), true, [&](const scenegraph::SceneGraphNode &node) {
		if (!node.isAnyModelNode() || node.volume() == nullptr) {
			return;
		}
		const voxel::Voxel voxel = node.volume()->voxel(0, 62, -576);
		if (node.name() == MCRFormat::WaterNodeName) {
			++waterNodes;
			const scenegraph::SceneGraphNode *parent = sceneGraph.parentNode(node);
			EXPECT_NE(parent, nullptr);
			EXPECT_TRUE(parent->isAnyModelNode() || parent->type() == scenegraph::SceneGraphNodeType::Root ||
						parent->type() == scenegraph::SceneGraphNodeType::Group);
			if (!voxel::isAir(voxel.getMaterial())) {
				foundWaterVoxel = true;
				EXPECT_TRUE(voxel::isTransparent(voxel.getMaterial())) << voxel;
				EXPECT_EQ(22u, voxel.getColor()) << voxel;
				EXPECT_LT(node.palette().color(22).a, 255u);
				EXPECT_EQ(node.palette().material(22).type, palette::MaterialType::Glass);
			}
			return;
		}
		if (!voxel::isAir(voxel.getMaterial()) && voxel.getColor() == 22) {
			foundWaterInTerrain = true;
		}
	});
	EXPECT_GT(waterNodes, 0);
	EXPECT_TRUE(foundWaterVoxel);
	EXPECT_FALSE(foundWaterInTerrain);
}

TEST_F(MCRFormatTest, testExtractWaterVolume) {
	const voxel::Region region(0, 0, 0, 3, 3, 3);
	voxel::RawVolume terrain(region);
	terrain.setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 5));
	terrain.setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Transparent, 22));
	voxel::RawVolume *water = MCRFormat::extractWaterVolume(&terrain);
	ASSERT_NE(water, nullptr);
	EXPECT_TRUE(voxel::isAir(terrain.voxel(2, 2, 2).getMaterial()));
	EXPECT_EQ(5u, terrain.voxel(1, 1, 1).getColor());
	EXPECT_TRUE(voxel::isTransparent(water->voxel(2, 2, 2).getMaterial()));
	EXPECT_EQ(22u, water->voxel(2, 2, 2).getColor());
	delete water;
}

TEST_F(MCRFormatTest, testLoad110) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "minecraft_110.mca", 1024);
	const scenegraph::SceneGraphNode &node = *sceneGraph.begin(scenegraph::SceneGraphNodeType::Model);
	ASSERT_EQ(node.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::RawVolume *v = node.volume();
	EXPECT_EQ(23296, v->region().voxels());
}

TEST_F(MCRFormatTest, testLoad113) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "minecraft_113.mca", 1024);
	const scenegraph::SceneGraphNode &node = *sceneGraph.begin(scenegraph::SceneGraphNodeType::Model);
	ASSERT_EQ(node.type(), scenegraph::SceneGraphNodeType::Model);
	const voxel::RawVolume *v = node.volume();
	EXPECT_EQ(17920, v->region().voxels());
}

TEST_F(MCRFormatTest, testPackedBlockStatesBitUnpackingNewFormatCrossBoundary) {
	TestMCRFormat fmt;

	palette::Palette pal;
	pal.minecraft();

	// New-format packed block states decode uses secPal.numBits to unpack palette indices.
	// We intentionally choose a bit width that is smaller than 64 bits and crosses 64-bit word boundaries often.
	TestMCRFormat::MinecraftSectionPalette secPal;
	secPal.pal.resize(2u);
	secPal.pal[0] = 0u; // air
	secPal.pal[1] = 1u; // any non-air palette index
	secPal.numBits = 4u;
	// Keep mcpal empty so parseBlockStates treats it as "samePalette".
	secPal.mcpal = palette::Palette();

	constexpr int blockCount = TestMCRFormat::MAX_SIZE * TestMCRFormat::MAX_SIZE * TestMCRFormat::MAX_SIZE;
	const size_t bitSize = secPal.numBits;
	ASSERT_TRUE(bitSize > 0u);

	const size_t totalBits = (size_t)blockCount * bitSize;
	const size_t wordCount = (totalBits + 63u) / 64u;

	core::Buffer<int64_t> packed(wordCount);
	// Set two known palette indices to 1. Everything else stays 0 (air).
	const int nonAir1 = 0;
	const int nonAir2 = blockCount - 1;
	auto setBitPackedIndex = [&](int blockIndex, uint64_t value) {
		const size_t bitPos = (size_t)blockIndex * bitSize;
		const size_t wordIndex = bitPos / 64u;
		const size_t bitOffset = bitPos % 64u;
		ASSERT_TRUE(wordIndex < packed.size());

		packed[wordIndex] |= (int64_t)(value << bitOffset);
		if (bitOffset + bitSize > 64u) {
			// Spill into next word.
			ASSERT_TRUE(wordIndex + 1u < packed.size());
			packed[wordIndex + 1u] |= (int64_t)(value >> (64u - bitOffset));
		}
	};

	setBitPackedIndex(nonAir1, 1u);
	setBitPackedIndex(nonAir2, 1u);

	const priv::NamedBinaryTag data(core::move(packed));

	TestMCRFormat::SectionVolumes volumes;
	ASSERT_TRUE(fmt.parseBlockStates(3000, pal, data, volumes, 0, secPal));
	ASSERT_EQ(1u, (size_t)volumes.size());

	const voxel::RawVolume *v = volumes[0];
	EXPECT_TRUE(voxel::isAir(v->voxel(1, 0, 0).getMaterial()));
	EXPECT_EQ(0u, v->voxel(1, 0, 0).getColor());
	EXPECT_FALSE(voxel::isAir(v->voxel(0, 0, 0).getMaterial()));
	EXPECT_EQ(1u, v->voxel(0, 0, 0).getColor());
	EXPECT_FALSE(voxel::isAir(v->voxel(15, 15, 15).getMaterial()));
	EXPECT_EQ(1u, v->voxel(15, 15, 15).getColor());

	// parseBlockStates allocates RawVolume; clean up to avoid leaking memory in the unit test.
	delete volumes[0];
}

} // namespace voxelformat
