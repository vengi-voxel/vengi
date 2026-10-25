/**
 * @file
 */

#include "voxelformat/private/slab6/KV6Format.h"
#include "voxelformat/private/slab6/KFAFormat.h"
#include "AbstractFormatTest.h"
#include "core/FourCC.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include "color/RGBA.h"
#include "palette/Palette.h"
#include "palette/NormalPalette.h"
#include "voxel/RawVolume.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Region.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/gtc/epsilon.hpp>

namespace voxelformat {

class KV6FormatTest : public AbstractFormatTest {};

TEST_F(KV6FormatTest, testNormalIndicesAndCoordinates) {
	palette::NormalPalette palette;
	palette.slab6();
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.createVolume(voxel::Region(0, 0, 0, 2, 0, 0));
	node.setNormalPalette(palette);
	node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 42, NORMAL_PALETTE_OFFSET));
	node.volume()->setVoxel(1, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 42, palette.size()));
	node.volume()->setVoxel(2, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 42));
	graph.emplace(core::move(node));
	graph.updateTransforms();
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	KV6Format format;
	ASSERT_TRUE(format.save(graph, "normals.kv6", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.load("normals.kv6", archive, loaded, testLoadCtx));
	const scenegraph::SceneGraphNode *model = loaded.firstModelNode();
	ASSERT_NE(nullptr, model);
	EXPECT_EQ(NORMAL_PALETTE_OFFSET, model->volume()->voxel(0, 0, 0).getNormal());
	EXPECT_EQ(palette.size(), model->volume()->voxel(1, 0, 0).getNormal());
	EXPECT_EQ(NO_NORMAL, model->volume()->voxel(2, 0, 0).getNormal());
	ASSERT_TRUE(model->hasNormalPalette());
	EXPECT_EQ(palette.hash(), model->normalPalette().hash());
}

TEST_F(KV6FormatTest, testRemapNormalPalettes) {
	for (const char *name : palette::NormalPalette::builtIn) {
		palette::NormalPalette palette;
		ASSERT_TRUE(palette.load(name));
		scenegraph::SceneGraph graph;
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.createVolume(voxel::Region(0, 0, 0, (int)palette.size() - 1, 0, 0));
		node.setNormalPalette(palette);
		for (size_t i = 0; i < palette.size(); ++i) {
			node.volume()->setVoxel(i, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 42, i + NORMAL_PALETTE_OFFSET));
		}
		graph.emplace(core::move(node));
		graph.updateTransforms();
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		KV6Format format;
		ASSERT_TRUE(format.save(graph, "remap.kv6", archive, testSaveCtx));
		scenegraph::SceneGraph loaded;
		ASSERT_TRUE(format.load("remap.kv6", archive, loaded, testLoadCtx));
		const scenegraph::SceneGraphNode *model = loaded.firstModelNode();
		ASSERT_NE(nullptr, model);
		ASSERT_TRUE(model->hasNormalPalette());
		const palette::NormalPalette &target = model->normalPalette();
		for (size_t i = 0; i < palette.size(); ++i) {
			const uint8_t index = model->volume()->voxel(i, 0, 0).getNormal();
			ASSERT_NE(NO_NORMAL, index);
			ASSERT_LE(index, target.size());
			const glm::vec3 sourceNormal = glm::normalize(palette.normal3f(i));
			const glm::vec3 writtenNormal = glm::normalize(target.normal3f(index - NORMAL_PALETTE_OFFSET));
			EXPECT_GE(glm::dot(sourceNormal, writtenNormal), glm::cos(glm::radians(20.0f))) << name << " " << i;
			if (palette.hash() == target.hash()) {
				EXPECT_EQ(i + NORMAL_PALETTE_OFFSET, index);
			}
		}
	}
}

TEST_F(KV6FormatTest, testLoad) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test.kv6", 1);
	if (HasFatalFailure() || IsSkipped()) {
		return;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const voxel::Region expectedRegion(0, 0, 0, 2, 2, 0);
	EXPECT_EQ(expectedRegion, node->region());
}

TEST_F(KV6FormatTest, testSaveCubeModel) {
	KV6Format f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testSaveLoadCube("kv6-savecubemodel.kv6", &f, flags);
}

TEST_F(KV6FormatTest, testSaveSmallVoxel) {
	KV6Format f;
	const voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testSaveLoadVoxel("kv6-smallvolumesavetest.kv6", &f, -16, 15, flags);
}

TEST_F(KV6FormatTest, testLoadSave) {
	KV6Format f;
	voxel::ValidateFlags flags = voxel::ValidateFlags::AllPaletteMinMatchingColors;
	testConvert("voxlap5.kv6", f, "kv6-voxlap5.kv6", f, flags);
}

TEST_F(KV6FormatTest, testAnasplit) {
	struct Object {
		voxel::Region region;
		int cnt;
	} modelRegions[] {
		{voxel::Region{0, 103, 9, 10, 143, 18}, 910},
		{voxel::Region{3, 148, 9, 16, 190, 23}, 1198},
		{voxel::Region{20, 71, 4, 39, 106, 27}, 2257},
		{voxel::Region{20, 110, 0, 61, 143, 27}, 4260},
		{voxel::Region{20, 148, 4, 61, 193, 33}, 5018},
		{voxel::Region{22, 18, 4, 36, 66, 24}, 1927},
		{voxel::Region{24, 0, 6, 36, 13, 34}, 1051},
		{voxel::Region{27, 198, 6, 54, 228, 33}, 2172},
		{voxel::Region{42, 71, 4, 61, 106, 27}, 2257},
		{voxel::Region{45, 0, 6, 57, 13, 34}, 1051},
		{voxel::Region{45, 18, 4, 59, 66, 24}, 1927},
		{voxel::Region{65, 148, 9, 78, 190, 23}, 1198},
		{voxel::Region{71, 103, 9, 81, 143, 18}, 910}
	};

	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "anasplit.kv6", lengthof(modelRegions));
	if (HasFatalFailure() || IsSkipped()) {
		return;
	}

	int compared = 0;
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (!node.isModelNode()) {
			continue;
		}
		const voxel::Region &region = node.region();
		bool found = false;
		for (const Object &r : modelRegions) {
			if (r.region != region) {
				continue;
			}
			EXPECT_EQ(r.cnt, voxelutil::countVoxels(*node.volume()));
			++compared;
			found = true;
			break;
		}
		EXPECT_TRUE(found) << "Expected region not found: voxel::Region{" << region.getLowerX() << ", " << region.getLowerY() << ", " << region.getLowerZ() << ", " << region.getUpperX() << ", " << region.getUpperY() << ", " << region.getUpperZ() << "}, "
			<< voxelutil::countVoxels(*node.volume());
	}
	EXPECT_EQ(lengthof(modelRegions), compared);
}

TEST_F(KV6FormatTest, testSaveLoadTranslationBakedIntoPivot) {
	KV6Format f;
	const glm::vec3 translation(4.0f, 8.0f, -2.0f);
	const glm::vec3 dims(8.0f);
	testSaveLoadBakedPivot("kv6-translation-pivot.kv6", &f, glm::vec3(0.0f), translation, -translation / dims);
}

TEST_F(KV6FormatTest, testSaveLoadRaisedCenterPivot) {
	KV6Format f;
	testSaveLoadBakedPivot("kv6-raised-center-pivot.kv6", &f, glm::vec3(0.5f), glm::vec3(0.0f, 4.0f, 0.0f),
						   glm::vec3(0.5f, 0.0f, 0.5f));
}

TEST_F(KV6FormatTest, testSaveLoadCenteredAabbTranslation) {
	KV6Format f;
	testSaveLoadBakedPivot("kv6-centered-aabb.kv6", &f, glm::vec3(0.0f), glm::vec3(-4.0f, 0.0f, -4.0f),
						   glm::vec3(0.5f, 0.0f, 0.5f));
}

TEST_F(KV6FormatTest, testSaveLoadDirtyWorldTranslation) {
	KV6Format f;
	testSaveLoadBakedPivot("kv6-dirty-translation.kv6", &f, glm::vec3(0.0f), glm::vec3(-4.0f, 0.0f, -4.0f),
						   glm::vec3(0.5f, 0.0f, 0.5f), false);
}

static bool writeTestKfa(io::SeekableWriteStream &stream, const core::String &kv6Name, int16_t frame1ChildAngle,
						 bool stopMarkerAtZero = false) {
	if (!stream.writeUInt32(FourCC('K', 'w', 'l', 'k'))) {
		return false;
	}
	if (!stream.writePascalStringUInt32LE(kv6Name)) {
		return false;
	}
	if (!stream.writeUInt32(2)) {
		return false;
	}
	if (!stream.writeInt32(-1)) {
		return false;
	}
	for (int i = 0; i < 12; ++i) {
		if (!stream.writeFloat(0.0f)) {
			return false;
		}
	}
	if (!stream.writeInt16(0) || !stream.writeInt16(0) || !stream.writeInt8(0)) {
		return false;
	}
	for (int i = 0; i < 7; ++i) {
		if (!stream.writeInt8(0)) {
			return false;
		}
	}
	if (!stream.writeInt32(0)) {
		return false;
	}
	if (!stream.writeFloat(0.0f) || !stream.writeFloat(0.0f) || !stream.writeFloat(0.0f)) {
		return false;
	}
	if (!stream.writeFloat(5.0f) || !stream.writeFloat(0.0f) || !stream.writeFloat(0.0f)) {
		return false;
	}
	if (!stream.writeFloat(0.0f) || !stream.writeFloat(0.0f) || !stream.writeFloat(1.0f)) {
		return false;
	}
	if (!stream.writeFloat(0.0f) || !stream.writeFloat(0.0f) || !stream.writeFloat(1.0f)) {
		return false;
	}
	if (!stream.writeInt16(0) || !stream.writeInt16(0) || !stream.writeInt8(0)) {
		return false;
	}
	for (int i = 0; i < 7; ++i) {
		if (!stream.writeInt8(0)) {
			return false;
		}
	}
	if (!stream.writeUInt32(2)) {
		return false;
	}
	if (!stream.writeInt16(0) || !stream.writeInt16(0)) {
		return false;
	}
	if (!stream.writeInt16(0) || !stream.writeInt16(frame1ChildAngle)) {
		return false;
	}
	if (stopMarkerAtZero) {
		if (!stream.writeUInt32(3)) {
			return false;
		}
		if (!stream.writeInt32(0) || !stream.writeInt32(-1)) {
			return false;
		}
		if (!stream.writeInt32(200) || !stream.writeInt32(0)) {
			return false;
		}
		if (!stream.writeInt32(1000) || !stream.writeInt32(1)) {
			return false;
		}
	} else {
		if (!stream.writeUInt32(2)) {
			return false;
		}
		if (!stream.writeInt32(0) || !stream.writeInt32(0)) {
			return false;
		}
		if (!stream.writeInt32(1000) || !stream.writeInt32(1)) {
			return false;
		}
	}
	return true;
}

TEST_F(KV6FormatTest, testKfaTwoLimbs) {
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	palette::Palette pal;
	pal.tryAdd(color::RGBA(255, 0, 0, 255));
	node.setPalette(pal);
	node.createVolume(voxel::Region(0, 0, 0, 5, 0, 0));
	node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	node.volume()->setVoxel(5, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	graph.emplace(core::move(node));
	graph.updateTransforms();

	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	KV6Format kv6;
	ASSERT_TRUE(kv6.save(graph, "twolimbs.kv6", archive, testSaveCtx));

	io::BufferedReadWriteStream kfaStream;
	ASSERT_TRUE(writeTestKfa(kfaStream, "twolimbs.kv6", 16384));
	ASSERT_TRUE(archive->add("twolimbs.kfa", kfaStream.getBuffer(), (size_t)kfaStream.size()));

	KFAFormat::Data parsed;
	ASSERT_TRUE(KFAFormat::parse("twolimbs.kfa", archive, parsed));
	ASSERT_EQ(2u, parsed.hinges.size());
	EXPECT_EQ(-1, parsed.hinges[0].parent);
	EXPECT_EQ(0, parsed.hinges[1].parent);
	ASSERT_EQ(2u, parsed.numFrames);
	ASSERT_EQ(2u, parsed.seq.size());
	EXPECT_EQ(24, KFAFormat::timeToFrame(1000));

	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(kv6.load("twolimbs.kv6", archive, loaded, testLoadCtx));
	EXPECT_EQ(2u, loaded.size());
	EXPECT_TRUE(loaded.hasAnimation(DEFAULT_ANIMATION));

	const scenegraph::SceneGraphNode *rootLimb = nullptr;
	const scenegraph::SceneGraphNode *childLimb = nullptr;
	for (const auto &e : loaded.nodes()) {
		const scenegraph::SceneGraphNode &n = e->value;
		if (!n.isModelNode()) {
			continue;
		}
		if (n.property("kfa.hinge") == "0") {
			rootLimb = &n;
		} else if (n.property("kfa.hinge") == "1") {
			childLimb = &n;
		}
	}
	ASSERT_NE(nullptr, rootLimb);
	ASSERT_NE(nullptr, childLimb);
	EXPECT_EQ(rootLimb->id(), loaded.parentId(*childLimb));
	EXPECT_GE(childLimb->keyFrames().size(), 2u);
	EXPECT_TRUE(childLimb->hasKeyFrame(0));
	EXPECT_TRUE(childLimb->hasKeyFrame(12));
	EXPECT_TRUE(childLimb->hasKeyFrame(24));
	// Velcro is on the child at Ken (5,0,0); origin must not stay at volume (0,0,0)
	EXPECT_FALSE(glm::all(glm::epsilonEqual(childLimb->pivot(), glm::vec3(0.0f), 0.0001f)));
}

TEST_F(KV6FormatTest, testKfaStopMarkerAtZero) {
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	palette::Palette pal;
	pal.tryAdd(color::RGBA(255, 0, 0, 255));
	node.setPalette(pal);
	node.createVolume(voxel::Region(0, 0, 0, 5, 0, 0));
	node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	node.volume()->setVoxel(5, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
	graph.emplace(core::move(node));
	graph.updateTransforms();

	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	KV6Format kv6;
	ASSERT_TRUE(kv6.save(graph, "twolimbs.kv6", archive, testSaveCtx));

	io::BufferedReadWriteStream kfaStream;
	ASSERT_TRUE(writeTestKfa(kfaStream, "twolimbs.kv6", 16384, true));
	ASSERT_TRUE(archive->add("twolimbs.kfa", kfaStream.getBuffer(), (size_t)kfaStream.size()));

	KFAFormat::Data parsed;
	ASSERT_TRUE(KFAFormat::parse("twolimbs.kfa", archive, parsed));
	ASSERT_EQ(3u, parsed.seq.size());
	EXPECT_EQ(-1, parsed.seq[0].frame);
	EXPECT_EQ(0, parsed.seq[0].time);

	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(kv6.load("twolimbs.kv6", archive, loaded, testLoadCtx));
	const scenegraph::SceneGraphNode *childLimb = nullptr;
	for (const auto &e : loaded.nodes()) {
		const scenegraph::SceneGraphNode &n = e->value;
		if (n.isModelNode() && n.property("kfa.hinge") == "1") {
			childLimb = &n;
		}
	}
	ASSERT_NE(nullptr, childLimb);
	EXPECT_TRUE(childLimb->hasKeyFrame(0));
	EXPECT_TRUE(childLimb->hasKeyFrame(5));
	const scenegraph::KeyFrameIndex kf0 = childLimb->keyFrameForFrame(0);
	const scenegraph::KeyFrameIndex kf5 = childLimb->keyFrameForFrame(5);
	ASSERT_NE(InvalidKeyFrame, kf0);
	ASSERT_NE(InvalidKeyFrame, kf5);
	EXPECT_EQ(childLimb->transform(kf0).localTranslation(), childLimb->transform(kf5).localTranslation());
}

TEST_F(KV6FormatTest, testAnasplitKfa) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "anasplit.kv6", 13);
	if (HasFatalFailure() || IsSkipped()) {
		return;
	}
	if (!sceneGraph.hasAnimation(DEFAULT_ANIMATION)) {
		GTEST_SKIP() << "anasplit.kfa was not applied";
	}
	int transformHinges = 0;
	int models = 0;
	bool foundRoot = false;
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (node.isModelNode()) {
			++models;
			EXPECT_FALSE(node.property("kfa.hinge").empty());
		} else if (node.type() == scenegraph::SceneGraphNodeType::Group && !node.property("kfa.hinge").empty()) {
			++transformHinges;
		}
		if (node.property("kfa.hinge") == "3") {
			foundRoot = true;
			EXPECT_EQ(0, sceneGraph.parentId(node));
			EXPECT_TRUE(node.hasKeyFrame(0));
		}
	}
	EXPECT_EQ(13, models);
	EXPECT_GE(transformHinges, 1);
	EXPECT_TRUE(foundRoot);

	const scenegraph::SceneGraphNode *hinge2 = nullptr;
	int pivotedModels = 0;
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (node.isModelNode() && !glm::all(glm::epsilonEqual(node.pivot(), glm::vec3(0.0f), 0.0001f))) {
			++pivotedModels;
		}
		if (node.property("kfa.hinge") == "2") {
			hinge2 = &node;
		}
	}
	EXPECT_GE(pivotedModels, 1);
	if (hinge2 != nullptr) {
		// seq 2208ms/pose1 -> 53, 2808ms/pose3 -> 67. Ken lerps hinge angles;
		// those in-between frames must be baked or the limbs fly apart.
		EXPECT_TRUE(hinge2->hasKeyFrame(53));
		EXPECT_TRUE(hinge2->hasKeyFrame(60));
		EXPECT_TRUE(hinge2->hasKeyFrame(67));
	}
}

} // namespace voxelformat
