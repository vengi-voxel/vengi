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
#include <glm/gtc/quaternion.hpp>
#include <glm/geometric.hpp>

namespace voxelformat {

class VMaxFormatTest : public AbstractFormatTest {};

TEST_F(VMaxFormatTest, testLoad) {
	ASSERT_TRUE(io::isA("0voxel.vmax.zip", voxelformat::voxelLoad()));
}

TEST_F(VMaxFormatTest, testLoadPaletteMaterials) {
	VMaxFormat f;
	palette::Palette palette;
	ASSERT_GT(helper_loadPalette("1voxel.vmax.zip", helper_filesystemarchive(), f, palette), 0);
	EXPECT_EQ("Palette #1", palette.name());
	EXPECT_NEAR(0.1f, palette.material(36).metal, 0.001f);
	EXPECT_NEAR(0.9f, palette.material(36).roughness, 0.001f);
	EXPECT_FALSE(palette.material(36).has(palette::MaterialEmit));
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
	EXPECT_NEAR(0.1f, node->palette().material(palIdx).metal, 0.001f);
	EXPECT_NEAR(0.9f, node->palette().material(palIdx).roughness, 0.001f);
}

TEST_F(VMaxFormatTest, DISABLED_testTransform) {
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

	// Official VoxelMax .vox is a padded encodeBuffers bake. After MagicaVoxel load,
	// world AABB matches in XZ; Y-max is 50 vs 49 because our cropped front-red
	// (t.y=28, height 23) sits one voxel higher than the official padded SIZE bake.
	const voxel::ValidateFlags flags = voxel::ValidateFlags::Color | voxel::ValidateFlags::Region;
	voxel::sceneGraphComparator(sceneGraphFromVmax, sceneGraphOfficial, flags);
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

} // namespace voxelformat
