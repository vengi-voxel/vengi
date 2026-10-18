/**
 * @file
 */

#include "voxelformat/private/magicavoxel/TrenchblocksFormat.h"
#include "AbstractFormatTest.h"
#include "color/ColorUtil.h"
#include "color/Distance.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "util/VarUtil.h"
#include "voxel/RawVolume.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/external/ogt_vox.h"
#include "voxelformat/private/aceofspades/AoSVXLFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

class TrenchblocksFormatTest : public AbstractFormatTest {
protected:
	const ogt_vox_scene *readScene(const io::ArchivePtr &archive, const char *filename) {
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
		if (!stream) {
			return nullptr;
		}
		core::Buffer<uint8_t> bytes;
		bytes.resize(stream->size());
		if (stream->read(bytes.data(), bytes.size()) != (int)bytes.size()) {
			return nullptr;
		}
		return ogt_vox_read_scene_with_flags(bytes.data(), bytes.size(),
											 k_read_scene_flags_groups |
												 k_read_scene_flags_keep_empty_models_instances |
												 k_read_scene_flags_keep_duplicate_models);
	}

	int addTerrain(scenegraph::SceneGraph &graph, const voxel::Region &region, bool solid = true) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(new voxel::RawVolume(region));
		palette::Palette palette;
		palette.setColor(0, color::RGBA(0, 0, 0, 255));
		node.setPalette(palette);
		if (solid) {
			node.volume()->setVoxel(region.getLowerCorner(), voxel::createVoxel(voxel::VoxelType::Generic, 0));
			node.volume()->setVoxel(region.getUpperCorner(), voxel::createVoxel(voxel::VoxelType::Generic, 0));
		}
		return graph.emplace(core::move(node));
	}
};

TEST_F(TrenchblocksFormatTest, testVXLToTrenchblocks) {
	AoSVXLFormat vxl;
	scenegraph::SceneGraph source;
	ASSERT_TRUE(vxl.load("aceofspades.vxl", helper_filesystemarchive(), source, testLoadCtx));
	ASSERT_EQ(source.size(), 1u);
	const scenegraph::SceneGraphNode &terrain = *source.firstModelNode();
	const voxel::Region &region = terrain.volume()->region();
	ASSERT_EQ(region.getDimensionsInVoxels(), glm::ivec3(512, 64, 512));

	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	const io::FormatDescription &desc = TrenchblocksFormat::format();
	ASSERT_TRUE(saveFormat(source, "map.vox", &desc, archive, testSaveCtx));
	const ogt_vox_scene *scene = readScene(archive, "map.vox");
	ASSERT_NE(scene, nullptr);
	EXPECT_EQ(scene->file_version, 200u);
	ASSERT_EQ(scene->num_models, 4u);
	ASSERT_EQ(scene->num_instances, 4u);
	const char *names[] = {"T1", "T2", "T3", "T4"};
	for (int i = 0; i < 4; ++i) {
		EXPECT_STREQ(scene->instances[i].name, names[i]);
		EXPECT_EQ(scene->instances[i].model_index, (uint32_t)i);
		EXPECT_EQ(scene->models[i]->size_x, 256u);
		EXPECT_EQ(scene->models[i]->size_y, 256u);
		EXPECT_EQ(scene->models[i]->size_z, 64u);
		EXPECT_EQ(scene->instances[i].transform.m30, -384 + (i % 2) * 256);
		EXPECT_EQ(scene->instances[i].transform.m31, i < 2 ? 384 : 128);
		EXPECT_EQ(scene->instances[i].transform.m32, 32);
	}
	ogt_vox_destroy_scene(scene);

	io::FileDescription fileDesc;
	fileDesc.set("map.vox");
	scenegraph::SceneGraph converted;
	ASSERT_TRUE(loadFormat(fileDesc, archive, converted, testLoadCtx));
	ASSERT_EQ(converted.size(), 4u);
	EXPECT_EQ(converted.size(scenegraph::SceneGraphNodeType::Point), 0u);
	EXPECT_EQ(converted.sceneRegion(), source.sceneRegion());
	auto distance = [](color::RGBA a, color::RGBA b) {
		return color::getDistance(a, b, color::Distance::Approximation);
	};
	float nearest[256];
	const palette::Palette &mapPalette = converted.firstPalette();
	for (int i = 0; i < terrain.palette().colorCount(); ++i) {
		nearest[i] = color::ApproximationDistanceMax;
		for (int j = 16; j < mapPalette.colorCount(); ++j) {
			if (mapPalette.color(j).a == 255) {
				nearest[i] = glm::min(nearest[i], distance(terrain.palette().color(i), mapPalette.color(j)));
			}
		}
	}
	for (auto iter = converted.beginModel(); iter != converted.end(); ++iter) {
		const scenegraph::SceneGraphNode &tile = *iter;
		SCOPED_TRACE(tile.name().c_str());
		const glm::ivec3 offset(tile.transform().worldTranslation());
		bool sameGeometry = true;
		bool validColors = true;
		voxelutil::visitVolume(
			*tile.volume(),
			[&](int x, int y, int z, const voxel::Voxel &voxel) {
				const voxel::Voxel original = terrain.volume()->voxel(offset + glm::ivec3(x, y, z));
				sameGeometry &= voxel::isAir(voxel.getMaterial()) == voxel::isAir(original.getMaterial());
				if (!voxel::isAir(voxel.getMaterial())) {
					const color::RGBA color = tile.palette().color(voxel.getColor());
					validColors &=
						voxel.getColor() >= 16 && color.a == 255 &&
						distance(color, terrain.palette().color(original.getColor())) == nearest[original.getColor()];
				}
			},
			voxelutil::VisitAll());
		EXPECT_TRUE(sameGeometry);
		EXPECT_TRUE(validColors);
	}
}

TEST_F(TrenchblocksFormatTest, testVXLTrenchblocksVXLRoundtrip) {
	AoSVXLFormat vxl;
	TrenchblocksFormat trenchblocks;
	scenegraph::SceneGraph source;
	ASSERT_TRUE(vxl.load("aceofspades.vxl", helper_filesystemarchive(), source, testLoadCtx));
	ASSERT_EQ(source.size(), 1u);

	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	ASSERT_TRUE(trenchblocks.save(source, "map.vox", archive, testSaveCtx));
	scenegraph::SceneGraph tiles;
	ASSERT_TRUE(trenchblocks.load("map.vox", archive, tiles, testLoadCtx));
	ASSERT_EQ(tiles.size(), 4u);

	ASSERT_TRUE(vxl.save(tiles, "roundtrip.vxl", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(vxl.load("roundtrip.vxl", archive, loaded, testLoadCtx));
	ASSERT_EQ(loaded.size(), 1u);
	const scenegraph::SceneGraphNode &src = *source.firstModelNode();
	const scenegraph::SceneGraphNode &dst = *loaded.firstModelNode();
	EXPECT_EQ(dst.region().getDimensionsInVoxels(), glm::ivec3(512, 64, 512));
	EXPECT_EQ(src.region().getDimensionsInVoxels(), dst.region().getDimensionsInVoxels());
	// AoS maps always keep a solid bottom layer; only assert source solids survived.
	int solidSrc = 0;
	int missing = 0;
	voxelutil::visitVolume(
		*src.volume(),
		[&](int x, int y, int z, const voxel::Voxel &) {
			++solidSrc;
			if (voxel::isAir(dst.volume()->voxel(x, y, z).getMaterial())) {
				++missing;
			}
		},
		voxelutil::VisitSolid());
	EXPECT_GT(solidSrc, 0);
	EXPECT_EQ(missing, 0);
}

TEST_F(TrenchblocksFormatTest, testCropAndValidateMaps) {
	TrenchblocksFormat format;
	for (const glm::ivec3 &size : {glm::ivec3(513, 1, 1), glm::ivec3(1, 257, 1), glm::ivec3(1, 1, 513)}) {
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		scenegraph::SceneGraph graph;
		const glm::ivec3 mins(-12, 7, -4);
		const glm::ivec3 clipped = glm::min(size, glm::ivec3(512, 256, 512));
		addTerrain(graph, voxel::Region(mins, mins + size - 1));
		scenegraph::SceneGraphNode &node = *graph.firstModelNode();
		node.volume()->setVoxel(mins + clipped - 1, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		node.transform().setLocalTranslation(glm::vec3(10, 20, 30));
		ASSERT_TRUE(format.save(graph, "cropped.vox", archive, testSaveCtx));
		const ogt_vox_scene *scene = readScene(archive, "cropped.vox");
		ASSERT_NE(scene, nullptr);
		ASSERT_EQ(scene->num_models, 4u);
		int solids = 0;
		for (uint32_t m = 0; m < scene->num_models; ++m) {
			const ogt_vox_model &model = *scene->models[m];
			EXPECT_EQ(model.size_z, (uint32_t)clipped.y);
			for (uint32_t i = 0; i < model.size_x * model.size_y * model.size_z; ++i) {
				solids += model.voxel_data[i] != 0;
			}
		}
		EXPECT_EQ(solids, 2); // Both boundary cells survive; the cell just beyond the box is discarded.
		EXPECT_EQ(scene->instances[2].transform.m30, -(mins.x + 10 + clipped.x) + 128);
		EXPECT_EQ(scene->instances[2].transform.m31, mins.z + 30 + 128);
		EXPECT_EQ(scene->instances[2].transform.m32, mins.y + 20 + clipped.y / 2);
		ogt_vox_destroy_scene(scene);
		EXPECT_FALSE(voxel::isAir(node.volume()->voxel(mins + size - 1).getMaterial()));
		node.volume()->setVoxel(mins, voxel::Voxel());
		node.volume()->setVoxel(mins + clipped - 1, voxel::Voxel());
		EXPECT_FALSE(format.save(graph, "outside-only.vox", archive, testSaveCtx));
	}
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	scenegraph::SceneGraph empty;
	addTerrain(empty, voxel::Region(0, 0, 0, 0, 0, 0), false);
	EXPECT_FALSE(format.save(empty, "empty.vox", archive, testSaveCtx));
	for (const glm::vec3 &position :
		 {glm::vec3(0, -1, 0), glm::vec3(0, 256, 0), glm::vec3(glm::uintBitsToFloat(0x7fc00000u), 0, 0)}) {
		scenegraph::SceneGraph graph;
		addTerrain(graph, voxel::Region(0, 0, 0, 0, 0, 0));
		scenegraph::SceneGraphNode point(scenegraph::SceneGraphNodeType::Point);
		point.setName("FlagRed");
		point.transform().setLocalTranslation(position);
		graph.emplace(core::move(point));
		EXPECT_FALSE(format.save(graph, "invalid.vox", archive, testSaveCtx));
	}
	scenegraph::SceneGraph graph;
	addTerrain(graph, voxel::Region(0, 0, 0, 0, 255, 0));
	scenegraph::SceneGraphNode point(scenegraph::SceneGraphNodeType::Point);
	point.setName("FlagRed");
	point.setProperty("trenchblocks_2d", "true");
	graph.emplace(core::move(point));
	EXPECT_FALSE(format.save(graph, "overflow.vox", archive, testSaveCtx));
}

TEST_F(TrenchblocksFormatTest, testPointPlacementAndSpawnFiltering) {
	// A listed layer enables filtering. The other solid column blocks spawns only.
	for (bool filter : {false, true}) {
		scenegraph::SceneGraph graph;
		addTerrain(graph, voxel::Region(0, 0, 0, 3, 1, 0));
		graph.firstModelNode()->setName("terrain");
		const int spawnLayer = addTerrain(graph, voxel::Region(1, 0, 0, 1, 0, 0));
		graph.node(spawnLayer).setName("spawns");
		graph.node(graph.root().id()).setProperty("LayersPlayerSpawn", filter ? " \tspawns\t , missing " : "missing");
		auto point = [&](const char *name, int x, int y, bool point2D) {
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Point);
			node.setName(name);
			node.transform().setLocalTranslation(glm::vec3(x, y, 0));
			if (point2D) {
				node.setProperty("trenchblocks_2d", "true");
			}
			graph.emplace(core::move(node));
		};
		point("SpawnRed", 0, 0, true);
		point("SpawnBlue", 3, 1, false);
		point("PickupHealth", 3, 0, true);
		point("FlagRed", 2, 0, true);		 // Empty column: placed at the floor.
		point("PickupSatchel", 1, 0, false); // Stamp overrides terrain.
		point("PickupAmmo", 1, 0, false);	 // Last stamp wins.
		const io::MemoryArchivePtr archive = io::openMemoryArchive();
		TrenchblocksFormat format;
		ASSERT_TRUE(format.save(graph, "points.vox", archive, testSaveCtx));
		const ogt_vox_scene *scene = readScene(archive, "points.vox");
		ASSERT_NE(scene, nullptr);
		const uint8_t *data = scene->models[2]->voxel_data;
		EXPECT_EQ(scene->models[2]->size_z, 3u);
		EXPECT_EQ(data[3 + 256 * 256], filter ? 0 : 2);
		if (filter) {
			EXPECT_GE(data[256 * 256], 17);
		} else {
			EXPECT_EQ(data[256 * 256], 4);
		}
		EXPECT_EQ(data[2 * 256 * 256], 5);
		EXPECT_EQ(data[1], 1);
		EXPECT_EQ(data[2], 6);
		ogt_vox_destroy_scene(scene);
	}
}

TEST_F(TrenchblocksFormatTest, testMarkerPointRoundTrip) {
	const char *names[] = {"FlagRed",	   "SpawnRed",	 "FlagBlue",	  "SpawnBlue",
						   "PickupHealth", "PickupAmmo", "PickupSatchel", "SpectatorSpawnPoint"};
	scenegraph::SceneGraph source;
	addTerrain(source, voxel::Region(0, 0, 0, 15, 1, 0));
	for (int i = 0; i < 8; ++i) {
		scenegraph::SceneGraphNode point(scenegraph::SceneGraphNodeType::Point);
		point.setName(names[i]);
		point.transform().setLocalTranslation(glm::vec3(i, 1, 0));
		if (i == 6) {
			scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group);
			group.setName(names[i]);
			const int groupId = source.emplace(core::move(group));
			point.setName("point");
			source.emplace(core::move(point), groupId);
		} else {
			source.emplace(core::move(point));
		}
	}
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	TrenchblocksFormat format;
	ASSERT_TRUE(format.save(source, "markers.vox", archive, testSaveCtx));
	// Trenchblocks detection and marker positions must be independent of generic VOX transform settings.
	for (const char *applyTransform : {"true", "false"}) {
		util::ScopedVarChange scoped(cfg::VoxformatMVApplyTransform, applyTransform);
		io::FileDescription fileDesc;
		fileDesc.set("markers.vox");
		scenegraph::SceneGraph imported;
		ASSERT_TRUE(loadFormat(fileDesc, archive, imported, testLoadCtx));
		ASSERT_EQ(imported.size(scenegraph::SceneGraphNodeType::Point), 8u);
		EXPECT_EQ(imported.size(), 4u);
		bool found[8]{};
		for (auto iter = imported.begin(scenegraph::SceneGraphNodeType::Point); iter != imported.end(); ++iter) {
			const scenegraph::SceneGraphNode &point = *iter;
			for (int i = 0; i < 8; ++i) {
				if (point.name() == names[i]) {
					EXPECT_FALSE(found[i]);
					found[i] = true;
					EXPECT_EQ(point.transform().worldTranslation(), glm::vec3(i, 1, 0));
				}
			}
		}
		for (bool present : found) {
			EXPECT_TRUE(present);
		}
		int terrainVoxels = 0;
		for (auto iter = imported.beginModel(); iter != imported.end(); ++iter) {
			const scenegraph::SceneGraphNode &node = *iter;
			const int occupied = voxelutil::visitVolume(*node.volume(), [&](int, int, int, const voxel::Voxel &voxel) {
				++terrainVoxels;
				EXPECT_GE(voxel.getColor(), 16); // Native palettes are zero based.
				EXPECT_EQ(node.palette().color(voxel.getColor()), color::RGBA(0, 0, 0, 255));
			});
			EXPECT_EQ(occupied, node.name() == "T3" ? 2 : 0);
		}
		EXPECT_EQ(terrainVoxels, 2);
		ASSERT_TRUE(format.save(imported, "roundtrip.vox", archive, testSaveCtx));
		const ogt_vox_scene *scene = readScene(archive, "roundtrip.vox");
		ASSERT_NE(scene, nullptr);
		for (int i = 0; i < 8; ++i) {
			// Imported map box includes all four padded tiles, and retains its origin.
			EXPECT_EQ(scene->models[2]->voxel_data[15 - i + 256 * 256], i + 1);
		}
		EXPECT_GE(scene->models[2]->voxel_data[256 * 256], 17); // Terrain black is not a marker.
		ogt_vox_destroy_scene(scene);
	}
}

TEST_F(TrenchblocksFormatTest, testGenericVoxRetainsMarkerColors) {
	scenegraph::SceneGraph graph;
	addTerrain(graph, voxel::Region(0, 0, 0, 1, 0, 0));
	scenegraph::SceneGraphNode point(scenegraph::SceneGraphNodeType::Point);
	point.setName("PickupSatchel");
	point.transform().setLocalTranslation(glm::vec3(0));
	graph.emplace(core::move(point));
	const io::MemoryArchivePtr archive = io::openMemoryArchive();
	TrenchblocksFormat format;
	ASSERT_TRUE(format.save(graph, "map.vox", archive, testSaveCtx));
	const ogt_vox_scene *scene = readScene(archive, "map.vox");
	ASSERT_NE(scene, nullptr);
	EXPECT_TRUE(TrenchblocksFormat::isTrenchblocksScene(scene));
	ogt_vox_scene generic = *scene;
	ogt_vox_instance instances[4];
	core_memcpy(instances, scene->instances, sizeof(instances));
	instances[0].name = "Generic model";
	generic.instances = instances;
	EXPECT_FALSE(TrenchblocksFormat::isTrenchblocksScene(&generic));
	uint32_t length = 0;
	uint8_t *buffer = ogt_vox_write_scene(&generic, &length);
	ASSERT_NE(buffer, nullptr);
	ASSERT_TRUE(archive->add("generic.vox", buffer, length));
	ogt_vox_free(buffer);
	ogt_vox_destroy_scene(scene);
	VoxFormat vox;
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(vox.load("generic.vox", archive, loaded, testLoadCtx));
	EXPECT_EQ(loaded.size(scenegraph::SceneGraphNodeType::Point), 0u);
	int markers = 0;
	for (auto iter = loaded.beginModel(); iter != loaded.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		voxelutil::visitVolume(*node.volume(), [&](int, int, int, const voxel::Voxel &voxel) {
			if (voxel.getColor() == 6) {
				++markers;
			}
		});
	}
	EXPECT_EQ(markers, 1);
}

} // namespace voxelformat
