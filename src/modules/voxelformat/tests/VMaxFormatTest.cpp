/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "core/collection/Buffer.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "util/VarUtil.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/voxelmax/VMaxFormat.h"
#include "voxelformat/tests/TestHelper.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/geometric.hpp>

namespace voxelformat {

class VMaxFormatTest : public AbstractFormatTest {
protected:
	core::ScopedPtr<io::SeekableReadStream> _regressionStream;
	io::ArchivePtr regressionArchive();
};

TEST_F(VMaxFormatTest, testLoad) {
	ASSERT_TRUE(io::isA("0voxel.vmax.zip", voxelformat::voxelLoad()));
}

TEST_F(VMaxFormatTest, testLoadPaletteMaterials) {
	VMaxFormat f;
	palette::Palette palette;
	ASSERT_GT(helper_loadPalette("1voxel.vmax.zip", helper_filesystemarchive(), f, palette), 0);
	EXPECT_EQ("Palette #1", palette.name());
	EXPECT_NEAR(0.0f, palette.material(35).metal, 0.001f);
	EXPECT_NEAR(1.0f, palette.material(35).roughness, 0.001f);
	EXPECT_FALSE(palette.material(35).has(palette::MaterialEmit));
}

TEST_F(VMaxFormatTest, testLoadAppliesLayerMaterialToVoxelColor) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "1voxel.vmax.zip", 1);
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	uint8_t palIdx = 0;
	int voxels = 0;
	voxelutil::visitVolume(*node->volume(), [&](int, int, int, const voxel::Voxel &voxel) {
		if (!voxel::isAir(voxel.getMaterial())) {
			palIdx = voxel.getColor();
			++voxels;
		}
	});
	ASSERT_EQ(1, voxels);
	EXPECT_NEAR(0.0f, node->palette().material(palIdx).metal, 0.001f);
	EXPECT_NEAR(1.0f, node->palette().material(palIdx).roughness, 0.001f);
}

TEST_F(VMaxFormatTest, testTransform) {
	// test-transform.vox is VoxelMax's MagicaVoxel export of the same scene as test-transform.vmax.zip.
	// Load vmax with node TRS, write MagicaVoxel, then compare that vox graph to the official export.
	scenegraph::SceneGraph sceneGraphVMAX;
	testLoad(sceneGraphVMAX, "test-transform.vmax.zip", 20);

	sceneGraphVMAX.updateTransforms();
	const core::String exported = "test-transform-from-vmax.vox";
	{
		// Keep node rotations in the vox nTRN so MagicaVoxel load can bake them.
		// VoxelMax's own .vox already has baked voxels; default applyTransform
		// on load makes both graphs comparable in world space.
		util::ScopedVarChange apply(cfg::VoxformatMVApplyTransform, "false");
		ASSERT_TRUE(helper_saveSceneGraph(sceneGraphVMAX, exported));
	}

	scenegraph::SceneGraph sceneGraphFromVmax;
	testLoad(sceneGraphFromVmax, exported, 20);

	scenegraph::SceneGraph sceneGraphOfficial;
	testLoad(sceneGraphOfficial, "test-transform.vox", 20);

	// Official VoxelMax .vox is a padded encodeBuffers bake, so local volume
	// regions and per-voxel colors are not identical. Occupied world AABB should
	// still match in XZ; Y-max can differ by one because our cropped front-red
	// (t.y=28, height 23) sits one voxel higher than the official padded SIZE bake.
	EXPECT_EQ(sceneGraphOfficial.size(scenegraph::SceneGraphNodeType::AllModels),
			  sceneGraphFromVmax.size(scenegraph::SceneGraphNodeType::AllModels));
	sceneGraphOfficial.updateTransforms();
	sceneGraphFromVmax.updateTransforms();
	glm::vec3 officialMins(0.0f);
	glm::vec3 officialMaxs(0.0f);
	glm::vec3 vmaxMins(0.0f);
	glm::vec3 vmaxMaxs(0.0f);
	ASSERT_TRUE(voxel::occupiedWorldAABB(sceneGraphOfficial, officialMins, officialMaxs));
	ASSERT_TRUE(voxel::occupiedWorldAABB(sceneGraphFromVmax, vmaxMins, vmaxMaxs));
	EXPECT_NEAR(officialMins.x, vmaxMins.x, 1.0f);
	EXPECT_NEAR(officialMins.z, vmaxMins.z, 1.0f);
	EXPECT_NEAR(officialMaxs.x, vmaxMaxs.x, 1.0f);
	EXPECT_NEAR(officialMaxs.z, vmaxMaxs.z, 1.0f);
	EXPECT_NEAR(officialMaxs.y, vmaxMaxs.y, 1.0f);
}

TEST_F(VMaxFormatTest, testHierarchyAndAxisAngle) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "test-transform.vmax.zip", 20);
	sceneGraph.updateTransforms();

	const scenegraph::SceneGraphNode *topLeft = sceneGraph.findNodeByName("topLeft");
	const scenegraph::SceneGraphNode *topRight = sceneGraph.findNodeByName("topRight");
	const scenegraph::SceneGraphNode *bottomLeft = sceneGraph.findNodeByName("bottomLeft");
	const scenegraph::SceneGraphNode *bottomRight = sceneGraph.findNodeByName("bottomRight");
	ASSERT_NE(nullptr, topLeft);
	ASSERT_NE(nullptr, topRight);
	ASSERT_NE(nullptr, bottomLeft);
	ASSERT_NE(nullptr, bottomRight);

	int originalCount = 0;
	int cloneCount = 0;
	const scenegraph::SceneGraphNode *clone = nullptr;
	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::Model); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (node.name() == "original") {
			++originalCount;
			const scenegraph::SceneGraphNode *parent = sceneGraph.parentNode(node);
			ASSERT_NE(nullptr, parent);
			EXPECT_EQ("weird", parent->name());
		} else if (node.name() == "clone") {
			++cloneCount;
			if (clone == nullptr) {
				clone = &node;
			}
		}
	}
	EXPECT_EQ(4, originalCount);
	EXPECT_EQ(4, cloneCount);
	ASSERT_NE(nullptr, clone);
	const glm::quat &cloneRot = clone->transform(0).localOrientation();
	EXPECT_GT(glm::angle(cloneRot), 1.0f) << "clone t_r is 90 degrees around X in VoxelMax";
}

TEST_F(VMaxFormatTest, testAxisAngleYSwapPreservesParentSpaceCenter) {
	// Robots.vmax child: 90 deg around -Z. VXTransform.updateMatrix is T*R*S in
	// Z-up, so parentSpaceCenter = t_p + R*e_c is compact. Remap (x,y,z)->(x,z,y)
	// has det -1 and must conjugate as R(C*axis, -angle) or the part explodes.
	const glm::vec3 t_p(-112.0001f, 142.4999f, 0.0f);
	const glm::vec3 e_c(142.5f, 112.0f, 10.0f);
	const glm::vec3 axis(0.0f, 0.0f, -1.0f);
	const float angle = 1.5707955f;
	const glm::vec3 pscVmax = t_p + glm::mat3_cast(glm::angleAxis(angle, axis)) * e_c;
	EXPECT_NEAR(0.0f, pscVmax.x, 0.5f);
	EXPECT_NEAR(0.0f, pscVmax.y, 0.5f);
	EXPECT_NEAR(10.0f, pscVmax.z, 0.5f);

	const auto remap = [](const glm::vec3 &v) { return glm::vec3(v.x, v.z, v.y); };
	const glm::vec3 pscVengi = remap(pscVmax);
	const glm::vec3 axisVengi = glm::normalize(remap(axis));
	const glm::vec3 exploded = remap(t_p) + glm::mat3_cast(glm::angleAxis(angle, axisVengi)) * remap(e_c);
	const glm::vec3 assembled = remap(t_p) + glm::mat3_cast(glm::angleAxis(-angle, axisVengi)) * remap(e_c);
	EXPECT_GT(glm::length(exploded - pscVengi), 100.0f);
	EXPECT_NEAR(pscVengi.x, assembled.x, 0.5f);
	EXPECT_NEAR(pscVengi.y, assembled.y, 0.5f);
	EXPECT_NEAR(pscVengi.z, assembled.z, 0.5f);
}

TEST_F(VMaxFormatTest, testSingleVoxelAtWorldOrigin) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "1voxel.vmax.zip", 1);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
	sceneGraph.updateTransforms();

	glm::ivec3 voxelPos(0);
	int voxels = 0;
	voxelutil::visitVolume(*node->volume(), [&](int x, int y, int z, const voxel::Voxel &voxel) {
		if (!voxel::isAir(voxel.getMaterial())) {
			voxelPos = glm::ivec3(x, y, z);
			++voxels;
		}
	});
	ASSERT_EQ(1, voxels);
	const glm::mat4 worldMat = sceneGraph.worldMatrix(*node, 0);
	const glm::vec3 world = glm::vec3(worldMat * glm::vec4(voxelPos, 1.0f));
	EXPECT_NEAR(0.0f, world.x, 1.0f);
	EXPECT_NEAR(0.0f, world.y, 1.0f);
	EXPECT_NEAR(0.0f, world.z, 1.0f);
}

TEST_F(VMaxFormatTest, testLoad0) {
	// Node 'snapshots' is empty - this scene doesn't contain anything
	testLoad("0voxel.vmax.zip", 0);
}

TEST_F(VMaxFormatTest, testLoad1) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "1voxel.vmax.zip", 1);
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(voxelutil::countVoxels(*node->volume()), 1);
}

TEST_F(VMaxFormatTest, testLoad2) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "2voxel.vmax.zip");
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(voxelutil::countVoxels(*node->volume()), 2);
}

TEST_F(VMaxFormatTest, testLoad5) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "5voxel.vmax.zip");
	scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(node, nullptr);
	EXPECT_EQ(voxelutil::countVoxels(*node->volume()), 5);
}

TEST_F(VMaxFormatTest, testLoad5Screenshot) {
	color::RGBA color(251, 251, 251, 255);
	testLoadScreenshot("5voxel.vmax.zip", 1280, 1280, color, 1, 1);
}

static bool addArchiveFile(const io::ArchivePtr &from, const core::String &src, const io::MemoryArchivePtr &to,
						   const core::String &dst) {
	core::ScopedPtr<io::SeekableReadStream> in(from->readStream(src));
	if (!in) {
		return false;
	}
	const int64_t n = in->size();
	core::Buffer<uint8_t> buf;
	buf.resize((size_t)n);
	if (in->read(buf.data(), (size_t)n) != (int)n) {
		return false;
	}
	return to->add(dst, buf.data(), (size_t)n);
}

TEST_F(VMaxFormatTest, testLoadStandaloneVMaxb) {
	io::ArchivePtr fs = helper_filesystemarchive();
	core::ScopedPtr<io::SeekableReadStream> zipStream(fs->readStream("1voxel.vmax.zip"));
	ASSERT_NE(nullptr, (io::SeekableReadStream *)zipStream);
	io::ArchivePtr zip = io::openZipArchive(zipStream);
	ASSERT_NE(nullptr, zip.get());

	io::MemoryArchivePtr contents = io::openMemoryArchive();
	ASSERT_TRUE(addArchiveFile(zip, "contents.vmaxb", contents, "contents.vmaxb"));
	ASSERT_TRUE(addArchiveFile(zip, "palette.png", contents, "palette.png"));

	VMaxFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("contents.vmaxb", contents, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
	EXPECT_EQ(1, voxelutil::countVoxels(*node->volume()));
}

TEST_F(VMaxFormatTest, testLoadStandaloneVMaxbIndexedPalette) {
	io::ArchivePtr fs = helper_filesystemarchive();
	core::ScopedPtr<io::SeekableReadStream> zipStream(fs->readStream("1voxel.vmax.zip"));
	ASSERT_NE(nullptr, (io::SeekableReadStream *)zipStream);
	io::ArchivePtr zip = io::openZipArchive(zipStream);
	ASSERT_NE(nullptr, zip.get());

	io::MemoryArchivePtr contents = io::openMemoryArchive();
	ASSERT_TRUE(addArchiveFile(zip, "contents.vmaxb", contents, "contents1.vmaxb"));
	ASSERT_TRUE(addArchiveFile(zip, "palette.png", contents, "palette1.png"));

	VMaxFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("contents1.vmaxb", contents, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(1, voxelutil::countVoxels(*node->volume()));
}

TEST_F(VMaxFormatTest, testIsAVmaxDirectoryExtension) {
	ASSERT_TRUE(io::isA("SheikhZayedGrandMosque.vmax", voxelformat::voxelLoad()));
}

TEST_F(VMaxFormatTest, testLoadVMaxbUsesSiblingSceneJson) {
	io::ArchivePtr fs = helper_filesystemarchive();
	core::ScopedPtr<io::SeekableReadStream> zipStream(fs->readStream("1voxel.vmax.zip"));
	ASSERT_NE(nullptr, (io::SeekableReadStream *)zipStream);
	io::ArchivePtr zip = io::openZipArchive(zipStream);
	ASSERT_NE(nullptr, zip.get());

	io::MemoryArchivePtr contents = io::openMemoryArchive();
	ASSERT_TRUE(addArchiveFile(zip, "contents.vmaxb", contents, "contents1.vmaxb"));
	ASSERT_TRUE(addArchiveFile(zip, "palette.png", contents, "palette1.png"));
	const core::String sceneJson =
		"{\"v\":4,\"objects\":[{\"data\":\"contents1.vmaxb\",\"pal\":\"palette1.png\",\"n\":\"from-scene\"}]}";
	ASSERT_TRUE(contents->add("scene.json", (const uint8_t *)sceneJson.c_str(), sceneJson.size()));
	const uint8_t dummy = 0;
	ASSERT_TRUE(contents->add("contents.vmaxb", &dummy, 0));

	VMaxFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("contents.vmaxb", contents, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ("from-scene", node->name());
	EXPECT_EQ(1, voxelutil::countVoxels(*node->volume()));
}

TEST_F(VMaxFormatTest, testLoadVmaxDirectoryPackage) {
	io::ArchivePtr fs = helper_filesystemarchive();
	core::ScopedPtr<io::SeekableReadStream> zipStream(fs->readStream("1voxel.vmax.zip"));
	ASSERT_NE(nullptr, (io::SeekableReadStream *)zipStream);
	io::ArchivePtr zip = io::openZipArchive(zipStream);
	ASSERT_NE(nullptr, zip.get());

	io::MemoryArchivePtr contents = io::openMemoryArchive();
	ASSERT_TRUE(addArchiveFile(zip, "contents.vmaxb", contents, "mosque.vmax/contents.vmaxb"));
	ASSERT_TRUE(addArchiveFile(zip, "palette.png", contents, "mosque.vmax/palette.png"));
	const core::String sceneJson =
		"{\"v\":4,\"objects\":[{\"data\":\"contents.vmaxb\",\"pal\":\"palette.png\",\"n\":\"dir-package\"}]}";
	ASSERT_TRUE(contents->add("mosque.vmax/scene.json", (const uint8_t *)sceneJson.c_str(), sceneJson.size()));

	VMaxFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("mosque.vmax", contents, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ("dir-package", node->name());
	EXPECT_EQ(1, voxelutil::countVoxels(*node->volume()));
}

// vmax-regression.zip contains raw binary plists with minimal chunk stats,
// a 255-color zero-based table plus transparent terminator, and two materials.
io::ArchivePtr VMaxFormatTest::regressionArchive() {
	io::ArchivePtr fs = helper_filesystemarchive();
	_regressionStream = fs->readStream("vmax-regression.zip");
	if (!_regressionStream) {
		ADD_FAILURE() << "Missing vmax-regression.zip";
		return io::openMemoryArchive();
	}
	return io::openZipArchive(_regressionStream);
}

TEST_F(VMaxFormatTest, testLatestSnapshotReplacesWholeChunk) {
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(f.load("latest.vmaxb", regressionArchive(), graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(1, voxelutil::countVoxels(*node->volume()));
	EXPECT_EQ(color::RGBA(2, 253, 6, 255), node->palette().color(node->volume()->voxel(0, 0, 0).getColor()));
	graph.updateTransforms();
	EXPECT_FLOAT_EQ(1.0f, node->transform(0).worldTranslation().x);
}

TEST_F(VMaxFormatTest, testLatestEmptySnapshotDeletesChunk) {
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	// Loading empty storage succeeds without reviving the previous chunk.
	EXPECT_TRUE(f.load("empty.vmaxb", regressionArchive(), graph, testLoadCtx));
	EXPECT_EQ(nullptr, graph.firstModelNode());
}

TEST_F(VMaxFormatTest, testOneBasedColorIndices) {
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(f.load("colors.vmaxb", regressionArchive(), graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(color::RGBA(0, 255, 0, 255), node->palette().color(node->volume()->voxel(0, 0, 0).getColor()));
	EXPECT_EQ(color::RGBA(254, 1, 250, 255), node->palette().color(node->volume()->voxel(1, 0, 0).getColor()));
}

TEST_F(VMaxFormatTest, testSameColorOnDifferentSelectedMaterialLayers) {
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(f.load("materials.vmaxb", regressionArchive(), graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const uint8_t first = node->volume()->voxel(0, 0, 0).getColor();
	const uint8_t second = node->volume()->voxel(1, 0, 0).getColor();
	EXPECT_NE(first, second);
	EXPECT_EQ(node->palette().color(first), node->palette().color(second));
	EXPECT_NEAR(0.0f, node->palette().material(first).metal, 0.001f);
	EXPECT_NEAR(1.0f, node->palette().material(first).roughness, 0.001f);
	EXPECT_NEAR(1.0f, node->palette().material(second).metal, 0.001f);
	EXPECT_NEAR(0.0f, node->palette().material(second).roughness, 0.001f);
}

TEST_F(VMaxFormatTest, testMaterialCombinationsExceedPaletteCapacity) {
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(f.load("many-materials.vmaxb", regressionArchive(), graph, testLoadCtx));
	EXPECT_EQ(2, graph.size(scenegraph::SceneGraphNodeType::Model));
	int count = 0;
	for (auto it = graph.begin(scenegraph::SceneGraphNodeType::Model); it != graph.end(); ++it) {
		count += voxelutil::countVoxels(*(*it).volume());
		EXPECT_LE((*it).palette().colorCount(), 256);
		voxelutil::visitVolume(*(*it).volume(), [&](int, int, int, const voxel::Voxel &v) {
			if (!voxel::isAir(v.getMaterial())) {
				EXPECT_EQ(255, (*it).palette().color(v.getColor()).a);
			}
		});
	}
	EXPECT_EQ(510, count);
}

TEST_F(VMaxFormatTest, testWorkAreaHidesStoredVoxels) {
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(f.load("workarea.vmaxb", regressionArchive(), graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(1, voxelutil::countVoxels(*node->volume()));
	graph.updateTransforms();
	EXPECT_FLOAT_EQ(1.0f, node->transform(0).worldTranslation().x);
}

TEST_F(VMaxFormatTest, test512Workspace) {
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(f.load("large.vmaxb", regressionArchive(), graph, testLoadCtx));
	graph.updateTransforms();
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_FLOAT_EQ(256.0f, node->transform(0).worldTranslation().x);
}

TEST_F(VMaxFormatTest, testLegacyColorAndMaterialChunks) {
	VMaxFormat f;
	const io::ArchivePtr archive = regressionArchive();
	for (const char *name : {"legacy0.vmaxb", "legacy1.vmaxb", "legacy-latest.vmaxb"}) {
		SCOPED_TRACE(name);
		scenegraph::SceneGraph graph;
		ASSERT_TRUE(f.load(name, archive, graph, testLoadCtx));
		const scenegraph::SceneGraphNode *node = graph.firstModelNode();
		ASSERT_NE(nullptr, node);
		EXPECT_EQ(core::String(name) == "legacy0.vmaxb" ? 2 : 1, voxelutil::countVoxels(*node->volume()));
		if (core::String(name) == "legacy1.vmaxb") {
			const uint8_t index = node->volume()->voxel(0, 0, 0).getColor();
			EXPECT_NEAR(1.0f, node->palette().material(index).metal, 0.001f);
			graph.updateTransforms();
			EXPECT_FLOAT_EQ(33.0f, node->transform(0).worldTranslation().x);
		}
	}
}

TEST_F(VMaxFormatTest, testRejectsInvalidContents) {
	VMaxFormat f;
	const io::ArchivePtr archive = regressionArchive();
	for (const char *name : {"bad-offset.vmaxb", "bad-order.vmaxb", "bad-chunk.vmaxb", "odd-data.vmaxb",
							 "overflow-data.vmaxb", "legacy-bad-count.vmaxb", "short-stats.vmaxb", "invalid.vmaxb"}) {
		SCOPED_TRACE(name);
		scenegraph::SceneGraph graph;
		EXPECT_FALSE(f.load(name, archive, graph, testLoadCtx));
		EXPECT_EQ(nullptr, graph.firstModelNode());
	}
}

TEST_F(VMaxFormatTest, testExtractIndexedStandalonePalette) {
	const io::ArchivePtr archive = regressionArchive();
	io::MemoryArchivePtr files = io::openMemoryArchive();
	ASSERT_TRUE(addArchiveFile(archive, "colors.vmaxb", files, "contents1.vmaxb"));
	ASSERT_TRUE(addArchiveFile(archive, "palette.png", files, "palette1.png"));
	VMaxFormat f;
	palette::Palette palette;
	ASSERT_GT(f.loadPalette("contents1.vmaxb", files, palette, testLoadCtx), 0);
	EXPECT_EQ(color::RGBA(0, 255, 0, 255), palette.color(0));
}

TEST_F(VMaxFormatTest, testPaletteSettingsColorsWithoutPng) {
	const io::ArchivePtr archive = regressionArchive();
	io::MemoryArchivePtr files = io::openMemoryArchive();
	ASSERT_TRUE(addArchiveFile(archive, "colors.vmaxb", files, "contents.vmaxb"));
	ASSERT_TRUE(addArchiveFile(archive, "palette.settings.vmaxpsb", files, "palette.settings.vmaxpsb"));
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(f.load("contents.vmaxb", files, graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_EQ(color::RGBA(0, 255, 0, 255), node->palette().color(node->volume()->voxel(0, 0, 0).getColor()));
}

TEST_F(VMaxFormatTest, testEmbeddedDispersionMaterial) {
	const io::ArchivePtr archive = regressionArchive();
	io::MemoryArchivePtr files = io::openMemoryArchive();
	ASSERT_TRUE(addArchiveFile(archive, "dispersion.vmaxb", files, "contents.vmaxb"));
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(f.load("contents.vmaxb", files, graph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	const palette::Material &material = node->palette().material(node->volume()->voxel(0, 0, 0).getColor());
	EXPECT_EQ(palette::MaterialType::Glass, material.type);
	EXPECT_NEAR(1.5f, material.indexOfRefraction, 0.001f);
}

TEST_F(VMaxFormatTest, testSceneDoesNotSilentlySkipInvalidObject) {
	const io::ArchivePtr archive = regressionArchive();
	io::MemoryArchivePtr files = io::openMemoryArchive();
	ASSERT_TRUE(addArchiveFile(archive, "colors.vmaxb", files, "contents.vmaxb"));
	ASSERT_TRUE(addArchiveFile(archive, "palette.png", files, "palette.png"));
	const core::String json = "{\"objects\":[{\"data\":\"contents.vmaxb\",\"pal\":\"palette.png\"},"
		"{\"data\":\"missing.vmaxb\",\"pal\":\"palette.png\"}]}";
	ASSERT_TRUE(files->add("scene.vmax/scene.json", (const uint8_t *)json.c_str(), json.size()));
	ASSERT_TRUE(addArchiveFile(archive, "colors.vmaxb", files, "scene.vmax/contents.vmaxb"));
	ASSERT_TRUE(addArchiveFile(archive, "palette.png", files, "scene.vmax/palette.png"));
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	EXPECT_FALSE(f.load("scene.vmax", files, graph, testLoadCtx));
}

TEST_F(VMaxFormatTest, testDenseChunk) {
	VMaxFormat f;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(f.load("dense.vmaxb", regressionArchive(), graph, testLoadCtx));
	ASSERT_NE(nullptr, graph.firstModelNode());
	EXPECT_EQ(32768, voxelutil::countVoxels(*graph.firstModelNode()->volume()));
}

} // namespace voxelformat
