/**
 * @file
 */

#include "voxelformat/private/mesh/GodotSceneFormat.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "io/MemoryArchive.h"
#include "palette/Material.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "util/VarUtil.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxel/RawVolume.h"
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

class GodotSceneFormatTest : public AbstractFormatTest {
protected:
	static bool add(const io::MemoryArchivePtr &archive, const core::String &name, const core::String &text) {
		return archive->add(name, (const uint8_t *)text.c_str(), text.size());
	}

	core::ScopedPtr<util::ScopedVarChange> _project;

	bool onInitApp() override {
		if (!AbstractFormatTest::onInitApp())
			return false;
		_project = new util::ScopedVarChange(cfg::VoxformatGodotProject, "false");
		return true;
	}

	void onCleanupApp() override {
		_project.deallocate();
		AbstractFormatTest::onCleanupApp();
	}

	static void addModel(scenegraph::SceneGraph &graph) {
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.createVolume(voxel::Region(0, 0));
		node.volume()->setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 0));
		graph.emplace(core::move(node));
	}

	static void compareAnimation(scenegraph::SceneGraph &a, scenegraph::SceneGraph &b, const char *name) {
		const auto *source = a.findNodeByName(name);
		const auto *target = b.findNodeByName(name);
		ASSERT_NE(nullptr, source);
		ASSERT_NE(nullptr, target);
		for (int frame : {0, 6, 11, 12, 13, 24}) {
			SCOPED_TRACE(core::String::format("%s frame %i", name, frame));
			const auto am = a.transformForFrame(*source, frame).worldMatrix();
			const auto bm = b.transformForFrame(*target, frame).worldMatrix();
			for (int c = 0; c < 4; ++c)
				for (int r = 0; r < 4; ++r)
					EXPECT_NEAR(am[c][r], bm[c][r], 0.0001f);
		}
	}
};

TEST_F(GodotSceneFormatTest, testExportMesh) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "rgb.qb");
	ASSERT_TRUE(helper_saveSceneGraph(sceneGraph, "exportrgb.escn"));
}

TEST_F(GodotSceneFormatTest, testCreateProject) {
	util::ScopedVarChange project(cfg::VoxformatGodotProject, "true");
	scenegraph::SceneGraph graph;
	addModel(graph);
	for (const char *extension : {"tscn", "escn"}) {
		const auto archive = io::openMemoryArchive();
		const core::String filename = core::String("new/scene \"quoted\".") + extension;
		GodotSceneFormat format;
		ASSERT_TRUE(format.saveGroups(graph, filename, archive, testSaveCtx));
		ASSERT_TRUE(archive->exists("new/project.godot"));
		EXPECT_FALSE(archive->exists("project.godot"));
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream("new/project.godot"));
		ASSERT_TRUE(stream);
		core::String text;
		ASSERT_TRUE(stream->readString((int)stream->size(), text));
		EXPECT_NE(core::String::npos, text.find("config_version=5"));
		EXPECT_NE(core::String::npos, text.find("config/name = \"scene \\\"quoted\\\"\""));
		EXPECT_NE(core::String::npos,
				  text.find(core::String("run/main_scene = \"res://scene \\\"quoted\\\".") + extension + "\""));
	}
}

TEST_F(GodotSceneFormatTest, testExportProjectWithTextures) {
	util::ScopedVarChange project(cfg::VoxformatGodotProject, "true");
	util::ScopedVarChange meshMode(cfg::VoxformatMeshMode, "3");
	scenegraph::SceneGraph graph;
	addModel(graph);
	const auto archive = io::openMemoryArchive();
	for (const char *extension : {"tscn", "escn"}) {
		const core::String directory = core::String("godot-project-") + extension + "-export/";
		const core::String filename = directory + "scene." + extension;
		GodotSceneFormat format;
		ASSERT_TRUE(format.saveGroups(graph, filename, archive, testSaveCtx));
		EXPECT_TRUE(archive->exists(directory + "project.godot"));
		EXPECT_TRUE(archive->exists(directory + "scene-1.png"));

	}
}

TEST_F(GodotSceneFormatTest, testProjectDisabled) {
	util::ScopedVarChange project(cfg::VoxformatGodotProject, "false");
	scenegraph::SceneGraph graph;
	addModel(graph);
	const auto archive = io::openMemoryArchive();
	GodotSceneFormat format;
	ASSERT_TRUE(format.saveGroups(graph, "scene.tscn", archive, testSaveCtx));
	EXPECT_FALSE(archive->exists("project.godot"));
}

TEST_F(GodotSceneFormatTest, testProjectWriteFailure) {
	class ProjectWriteFailureArchive : public io::MemoryArchive {
	public:
		io::SeekableWriteStream *writeStream(const core::String &filename) override {
			if (filename == "project.godot") {
				return nullptr;
			}
			return io::MemoryArchive::writeStream(filename);
		}
	};
	util::ScopedVarChange project(cfg::VoxformatGodotProject, "true");
	scenegraph::SceneGraph graph;
	addModel(graph);
	const io::ArchivePtr archive = core::make_shared<ProjectWriteFailureArchive>();
	GodotSceneFormat format;
	EXPECT_FALSE(format.saveGroups(graph, "scene.tscn", archive, testSaveCtx));
	EXPECT_FALSE(archive->exists("project.godot"));
}

TEST_F(GodotSceneFormatTest, testPreserveExistingProject) {
	util::ScopedVarChange project(cfg::VoxformatGodotProject, "true");
	scenegraph::SceneGraph graph;
	addModel(graph);
	for (const char *directory : {"", "project/", "project/scenes/"}) {
		const auto archive = io::openMemoryArchive();
		const core::String projectFilename = core::String(directory) + "project.godot";
		const core::String original = "config_version=5\n[application]\nrun/main_scene=\"res://existing.tscn\"\n";
		ASSERT_TRUE(add(archive, projectFilename, original));
		GodotSceneFormat format;
		ASSERT_TRUE(format.saveGroups(graph, "project/scenes/scene.tscn", archive, testSaveCtx));
		core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(projectFilename));
		ASSERT_TRUE(stream);
		core::String text;
		ASSERT_TRUE(stream->readString((int)stream->size(), text));
		EXPECT_EQ(original, text);
		if (core::String(directory) != "project/scenes/") {
			EXPECT_FALSE(archive->exists("project/scenes/project.godot"));
		}
	}
}

TEST_F(GodotSceneFormatTest, testSaveLoadCube) {
	GodotSceneFormat format;
	testSaveLoadCube("godot-cube.tscn", &format, voxel::ValidateFlags::IgnoreHollow);
}

TEST_F(GodotSceneFormatTest, testTexturedRoundTrip) {
	util::ScopedVarChange meshMode(cfg::VoxformatMeshMode, "3");
	testRGBSmallSaveLoad("rgb_small.qb", "godot-textured.tscn");
}

TEST_F(GodotSceneFormatTest, testPrimitiveMeshes) {
	const auto archive = io::openMemoryArchive();
	ASSERT_TRUE(
		add(archive, "box.tscn",
			"[gd_scene format=3]\n"
			"[sub_resource type=\"StandardMaterial3D\" id=\"mat\"]\nalbedo_color = Color(0, 1, 0, 1)\n"
			"[sub_resource type=\"BoxMesh\" id=\"mesh\"]\nsize = Vector3(2, 2, 2)\nmaterial = SubResource(\"mat\")\n"
			"[node name=\"Model\" type=\"MeshInstance3D\"]\nmesh = SubResource(\"mesh\")\n"));
	GodotSceneFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.loadGroups("box.tscn", archive, graph, testLoadCtx));
	const auto *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	EXPECT_GT(voxelutil::countVoxels(*node->volume()), 0);
	voxel::Voxel v;
	voxelutil::visitVolume(*node->volume(), [&v](int, int, int, const voxel::Voxel &voxel) { v = voxel; });
	EXPECT_EQ(color::RGBA(0, 255, 0, 255), node->palette().color(v.getColor()));
}

TEST_F(GodotSceneFormatTest, testMaterials) {
	scenegraph::SceneGraph graph;
	testMaterial(graph, "godot-materials.escn");
}

TEST_F(GodotSceneFormatTest, testHierarchyAndTransforms) {
	const auto archive = io::openMemoryArchive();
	const core::String text =
		"[gd_scene format=3]\n"
		"[node name=\"Root\" type=\"Node3D\"]\nposition = Vector3(10, 0, 0)\n"
		"[node name=\"Group\" type=\"Node3D\" parent=\".\"]\n"
		"transform = Transform3D(0, 1, 0, -1, 0, 0, 0, 0, 1, 2, 3, 4)\n"
		"[node name=\"Nested\" type=\"Node3D\" parent=\"Group\"]\nposition = Vector3(1, 0, 0)\n"
		"[node name=\"Model\" type=\"Marker3D\" parent=\"Group/Nested\"]\n"
		"visible = false\n"
		R"(metadata/note = "quote: \" and slash: \\")"
		"\n"
		"[node name=\"Camera\" type=\"Camera3D\" parent=\"Group\"]\nfov = 60\nnear = 0.2\nfar = 100\n"
		"[node name=\"Marker\" type=\"Marker3D\" parent=\"Group/Nested\"]\n";
	ASSERT_TRUE(add(archive, "scene.tscn", text));
	GodotSceneFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.loadGroups("scene.tscn", archive, graph, testLoadCtx));
	const auto *model = graph.findNodeByName("Model");
	ASSERT_NE(nullptr, model);
	EXPECT_FALSE(model->visible());
	EXPECT_EQ("Nested", graph.node(graph.parentId(*model)).name());
	EXPECT_FLOAT_EQ(12, model->transform(0).worldTranslation().x);
	EXPECT_FLOAT_EQ(2, model->transform(0).worldTranslation().y);
	EXPECT_FLOAT_EQ(4, model->transform(0).worldTranslation().z);
	const auto *cameraNode = graph.findNodeByName("Camera");
	ASSERT_NE(nullptr, cameraNode);
	const auto &camera = scenegraph::toCameraNode(*cameraNode);
	EXPECT_EQ(60, camera.fieldOfView());
	EXPECT_FLOAT_EQ(0.2f, camera.nearPlane());
	EXPECT_FLOAT_EQ(100, camera.farPlane());
	ASSERT_NE(nullptr, graph.findNodeByName("Marker"));
	EXPECT_TRUE(graph.findNodeByName("Marker")->isPointNode());

	ASSERT_TRUE(format.saveGroups(graph, "roundtrip.escn", archive, testSaveCtx));
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.loadGroups("roundtrip.escn", archive, loaded, testLoadCtx));
	const auto *roundtrip = loaded.findNodeByName("Model");
	ASSERT_NE(nullptr, roundtrip);
	EXPECT_EQ("Nested", loaded.node(loaded.parentId(*roundtrip)).name());
	EXPECT_FALSE(roundtrip->visible());
	EXPECT_EQ(model->property("note"), roundtrip->property("note"));
	for (int c = 0; c < 4; ++c) {
		for (int r = 0; r < 4; ++r) {
			EXPECT_NEAR(model->transform(0).worldMatrix()[c][r], roundtrip->transform(0).worldMatrix()[c][r], 0.0001f);
		}
	}
}

TEST_F(GodotSceneFormatTest, testExternalResourcesAndOverrides) {
	const auto archive = io::openMemoryArchive();
	ASSERT_TRUE(add(archive, "project.godot", "config_version=5\n"));
	ASSERT_TRUE(
		add(archive, "materials/red.tres",
			"[gd_resource type=\"StandardMaterial3D\" format=3]\n"
			"[resource]\nalbedo_color = Color(1, 0, 0, 0.5)\ntransparency = 1\nmetallic = 0.7\nroughness = 0.25\n"
			"metallic_specular = 0.4\nemission_enabled = true\nemission = Color(1, 1, 1, "
			"1)\nemission_energy_multiplier = 0.6\n"));
	ASSERT_TRUE(add(archive, "meshes/model.tres",
					"[gd_resource type=\"BoxMesh\" format=3]\n[resource]\nsize = Vector3(2, 2, 2)\n"));
	ASSERT_TRUE(add(archive, "scenes/scene.tscn",
					"[gd_scene format=3]\n"
					"[ext_resource type=\"BoxMesh\" path=\"res://meshes/model.tres\" id=\"mesh\"]\n"
					"[ext_resource type=\"Material\" path=\"res://materials/red.tres\" id=\"mat\"]\n"
					"[node name=\"Root\" type=\"Node3D\"]\n"
					"[node name=\"Model\" type=\"MeshInstance3D\" parent=\".\"]\n"
					"mesh = ExtResource(\"mesh\")\nmaterial_override = ExtResource(\"mat\")\n"));
	GodotSceneFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.loadGroups("scenes/scene.tscn", archive, graph, testLoadCtx));
	const auto *node = graph.firstModelNode();
	ASSERT_NE(nullptr, node);
	voxel::Voxel v;
	voxelutil::visitVolume(*node->volume(), [&v](int, int, int, const voxel::Voxel &voxel) { v = voxel; });
	const auto rgba = node->palette().color(v.getColor());
	EXPECT_EQ(255, rgba.r);
	EXPECT_EQ(0, rgba.g);
	EXPECT_NEAR(128, rgba.a, 1);
	const auto &mat = node->palette().material(v.getColor());
	EXPECT_FLOAT_EQ(0.7f, mat.metal);
	EXPECT_FLOAT_EQ(0.25f, mat.roughness);
	EXPECT_FLOAT_EQ(0.4f, mat.specular);
	EXPECT_FLOAT_EQ(0.6f, mat.emit);
}

TEST_F(GodotSceneFormatTest, testPackedSurface) {
	testLoad("godot-packed.tscn");
}

// Generated with Godot 4.4.1 ResourceSaver, including normal/tangent blocks and
// compressed positions and UVs. This fixture is independent of our writer.
TEST_F(GodotSceneFormatTest, testGodotGeneratedScene) {
	scenegraph::SceneGraph graph;
	testLoad(graph, "godot-scene.tscn", 2);
	const auto *plain = graph.findNodeByName("Uncompressed");
	const auto *compressed = graph.findNodeByName("Compressed");
	ASSERT_NE(nullptr, plain);
	ASSERT_NE(nullptr, compressed);
	EXPECT_EQ(plain->region(), compressed->region());
	EXPECT_EQ(voxelutil::countVoxels(*plain->volume()), voxelutil::countVoxels(*compressed->volume()));
	EXPECT_EQ("Group", graph.node(graph.parentId(*plain)).name());
	EXPECT_NEAR(5.0f, compressed->transform(0).worldTranslation().x, 0.0001f);
	EXPECT_NEAR(8.0f, compressed->transform(0).worldTranslation().y, 0.0001f);
	EXPECT_NEAR(5.0f, compressed->transform(0).worldTranslation().z, 0.0001f);
	voxel::Voxel v;
	voxelutil::visitVolume(*plain->volume(), [&v](int, int, int, const voxel::Voxel &voxel) { v = voxel; });
	const auto rgba = plain->palette().color(v.getColor());
	EXPECT_EQ(51, rgba.r);
	EXPECT_EQ(102, rgba.g);
	EXPECT_EQ(204, rgba.b);
	EXPECT_NEAR(128, rgba.a, 1);
	const auto &mat = plain->palette().material(v.getColor());
	EXPECT_FLOAT_EQ(0.7f, mat.metal);
	EXPECT_FLOAT_EQ(0.3f, mat.roughness);
	EXPECT_NEAR(0.54f, mat.emit, 0.003f);
}

// Godot 4.7 omits the default format=3 header attribute.
TEST_F(GodotSceneFormatTest, testCurrentGodotGeneratedScene) {
	testLoad("godot-scene-current.tscn", 2);
}

// Generated by Godot 4.7 with independent translation, rotation and scale keys,
// a nested AnimationPlayer and a named AnimationLibrary.
TEST_F(GodotSceneFormatTest, testImportGodotAnimation) {
	scenegraph::SceneGraph graph;
	testLoad(graph, "godot-animation.tscn");
	ASSERT_TRUE(graph.hasAnimation("Actions/Move"));
	ASSERT_TRUE(graph.hasAnimation("Actions/Step"));
	EXPECT_EQ("Actions/Move", graph.activeAnimation());
	EXPECT_EQ(nullptr, graph.findNodeByName("Player"));
	const auto *model = graph.findNodeByName("Box");
	ASSERT_NE(nullptr, model);
	EXPECT_EQ(glm::vec3(1, 2, 3), model->keyFrames("Actions/Move")[0].transform().localScale());
	const auto &midpoint = model->keyFrames("Actions/Move")[12].transform();
	EXPECT_EQ(glm::vec3(3, 4, 5), midpoint.localTranslation());
	EXPECT_EQ(glm::vec3(2, 3, 4), midpoint.localScale());
	EXPECT_NEAR(glm::sqrt(0.5f), glm::abs(midpoint.localOrientation().y), 0.0001f);
	const auto world = graph.transformForFrame(*model, 12);
	EXPECT_EQ(glm::vec3(15, 7, 9), world.worldTranslation());
	EXPECT_FLOAT_EQ(1, model->keyFrames("Actions/Step")[11].transform().localTranslation().x);
	EXPECT_FLOAT_EQ(9, model->keyFrames("Actions/Step")[12].transform().localTranslation().x);
	EXPECT_EQ(glm::vec3(1), model->keyFrames("Actions/Step")[12].transform().localScale());
	EXPECT_EQ("1", graph.root().property("godot_animation_loop/Actions/Step"));
}

TEST_F(GodotSceneFormatTest, testGodotAnimationRoundTrip) {
	scenegraph::SceneGraph graph;
	testLoad(graph, "godot-animation.tscn");
	GodotSceneFormat format;
	const auto archive = io::openMemoryArchive();
	for (const char *filename : {"godot-animated-roundtrip.tscn", "godot-animated-roundtrip.escn"}) {
		ASSERT_TRUE(format.saveGroups(graph, filename, archive, testSaveCtx));
		scenegraph::SceneGraph loaded;
		ASSERT_TRUE(format.loadGroups(filename, archive, loaded, testLoadCtx));
		for (const char *clip : {"Default", "Actions/Move", "Actions/Step"}) {
			ASSERT_TRUE(graph.setAnimation(clip));
			ASSERT_TRUE(loaded.setAnimation(clip));
			graph.updateTransforms();
			loaded.updateTransforms();
			for (const char *name : {"AnimatedScene", "Rig", "Box"}) {
				compareAnimation(graph, loaded, name);
			}
		}
		EXPECT_EQ("1", loaded.root().property("godot_animation_loop/Actions/Step"));
	}
}

TEST_F(GodotSceneFormatTest, testExportVengiAnimation) {
	scenegraph::SceneGraph graph;
	scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group);
	group.setName("Rig");
	group.transform(0).setLocalTranslation(glm::vec3(2, 3, 4));
	const int groupId = graph.emplace(core::move(group));
	ASSERT_NE(InvalidNodeId, groupId);

	scenegraph::SceneGraphNode marker(scenegraph::SceneGraphNodeType::Point);
	marker.setName("Marker");
	const int markerId = graph.emplace(core::move(marker), groupId);
	graph.updateTransforms();
	const core::String clip = "Wave: quoted \"clip\"";
	ASSERT_TRUE(graph.addAnimation(clip));
	for (const auto &entry : graph.nodes()) {
		auto &node = entry->value;
		node.setAnimation(clip);
		node.setKeyFrames(node.keyFrames("Default"));
	}
	ASSERT_TRUE(graph.setAnimation(clip));
	for (int nodeId : {groupId, markerId}) {
		auto &node = graph.node(nodeId);
		node.keyFrame(0).interpolation =
			nodeId == markerId ? scenegraph::InterpolationType::Instant : scenegraph::InterpolationType::QuadEaseIn;
		const auto first = node.transform(0);
		const int key = node.addKeyFrame(24);
		ASSERT_NE(InvalidKeyFrame, key);
		node.setTransform(key, first);
		node.transform(key).setLocalTranslation(first.localTranslation() + glm::vec3(4, 2, 0));
		node.transform(key).setLocalOrientation(glm::angleAxis(glm::radians(90.0f), glm::vec3(0, 1, 0)));
		node.transform(key).setLocalScale(glm::vec3(2));
	}
	graph.updateTransforms();
	GodotSceneFormat format;
	const auto archive = io::openMemoryArchive();
	ASSERT_TRUE(format.saveGroups(graph, "godot-vengi-animation.tscn", archive, testSaveCtx));
	EXPECT_EQ(clip, graph.activeAnimation());
	scenegraph::SceneGraph loaded;
	ASSERT_TRUE(format.loadGroups("godot-vengi-animation.tscn", archive, loaded, testLoadCtx));
	ASSERT_TRUE(loaded.setAnimation(clip));
	loaded.updateTransforms();
	for (const char *name : {"Rig", "Marker"})
		compareAnimation(graph, loaded, name);
}

TEST_F(GodotSceneFormatTest, testExternalAnimationResources) {
	const auto archive = io::openMemoryArchive();
	ASSERT_TRUE(add(archive, "clips/move.tres",
					"[gd_resource type=\"Animation\" format=3]\n[resource]\nlength = 1\n"
					"tracks/0/type = \"position_3d\"\ntracks/0/path = NodePath(\"Model\")\n"
					"tracks/0/keys = PackedFloat32Array(0, 2, 0, 0, 0, 1, 1, 8, 0, 0)\n"));
	ASSERT_TRUE(add(archive, "clips/library.tres",
					"[gd_resource type=\"AnimationLibrary\" format=3]\n"
					"[ext_resource type=\"Animation\" path=\"move.tres\" id=\"animation\"]\n[resource]\n_data = { "
					"&\"Ease\": ExtResource(\"animation\") }\n"));
	ASSERT_TRUE(
		add(archive, "scene.tscn",
			"[gd_scene format=3]\n"
			"[ext_resource type=\"AnimationLibrary\" path=\"clips/library.tres\" id=\"library\"]\n"
				"[node name=\"Root\" type=\"Node3D\"]\n"
				"[node name=\"Model\" type=\"Node3D\" parent=\".\"]\n"
				"[node name=\"Tools\" type=\"Node3D\" parent=\".\"]\n"
				"[node name=\"Player\" type=\"AnimationPlayer\" parent=\"Tools\"]\nroot_node = NodePath(\"../..\")\n"
				"libraries = { &\"\": ExtResource(\"library\") }\n"));
	GodotSceneFormat format;
	scenegraph::SceneGraph graph;
	ASSERT_TRUE(format.loadGroups("scene.tscn", archive, graph, testLoadCtx));
	ASSERT_TRUE(graph.hasAnimation("Ease"));
	const auto *model = graph.findNodeByName("Model");
	ASSERT_NE(nullptr, model);
	EXPECT_FLOAT_EQ(2, model->keyFrames("Ease")[12].transform().localTranslation().x);
	EXPECT_EQ(glm::vec3(1), model->keyFrames("Ease")[12].transform().localScale());
}

TEST_F(GodotSceneFormatTest, testMalformedAnimationTracks) {
	const core::String tracks[] = {
		"tracks/0/type = \"position_3d\"\ntracks/0/path = NodePath(\"Missing\")\ntracks/0/keys = PackedFloat32Array(0, "
		"1, 0, 0, 0)\n",
		"tracks/0/type = \"position_3d\"\ntracks/0/path = NodePath(\"Model\")\ntracks/0/keys = PackedFloat32Array(0, "
		"1, 0)\n",
		"tracks/0/type = \"rotation_3d\"\ntracks/0/path = NodePath(\"Model\")\ntracks/0/keys = PackedFloat32Array(0, "
		"1, 0, 0, 0, 0)\n",
		"tracks/0/type = \"scale_3d\"\ntracks/0/path = NodePath(\"Model\")\ntracks/0/keys = PackedFloat32Array(-1, 1, "
		"1, 1, 1)\n",
		"tracks/0/type = \"position_3d\"\ntracks/0/path = NodePath(\"Model\")\ntracks/0/keys = PackedFloat32Array(0, "
		"1, 0, 0, 0, 0, 1, 1, 1, 1)\n",
	};
	for (const auto &track : tracks) {
		const auto archive = io::openMemoryArchive();
		ASSERT_TRUE(add(
			archive, "bad.tscn",
			"[gd_scene format=3]\n" "[sub_resource type=\"Animation\" id=\"anim\"]\n" + track +
				"[sub_resource type=\"AnimationLibrary\" id=\"library\"]\n_data = {\"Bad\": SubResource(\"anim\")}\n"
				"[node name=\"Root\" type=\"Node3D\"]\n"
				"[node name=\"Model\" type=\"Node3D\" parent=\".\"]\n"
				"[node name=\"Player\" type=\"AnimationPlayer\" parent=\".\"]\nlibraries/ = "
				"SubResource(\"library\")\n"));
		GodotSceneFormat format;
		scenegraph::SceneGraph graph;
		EXPECT_FALSE(format.loadGroups("bad.tscn", archive, graph, testLoadCtx));
	}
}

TEST_F(GodotSceneFormatTest, testTransformNumericParsing) {
	const char *numbers[] = {"1.25e+1", "nan"};
	for (size_t i = 0; i < lengthof(numbers); ++i) {
		SCOPED_TRACE(numbers[i]);
		const auto archive = io::openMemoryArchive();
		ASSERT_TRUE(add(archive, "numeric.tscn",
						core::String("[gd_scene format=3]\n[node name=\"Root\" type=\"Node3D\"]\ntransform = "
									 "Transform3D(1, 0, 0, 0, 1, 0, 0, 0, 1, ") +
							numbers[i] + ", 0, 0)\n"));
		GodotSceneFormat format;
		scenegraph::SceneGraph graph;
		if (i == 0) {
			ASSERT_TRUE(format.loadGroups("numeric.tscn", archive, graph, testLoadCtx));
			const auto *node = graph.findNodeByName("Root");
			ASSERT_NE(nullptr, node);
			EXPECT_FLOAT_EQ(12.5f, node->transform().localTranslation().x);
		} else {
			EXPECT_FALSE(format.loadGroups("numeric.tscn", archive, graph, testLoadCtx));
		}
	}
}

TEST_F(GodotSceneFormatTest, testRejectMalformedScenes) {
	const char *scenes[] = {
		"[gd_scene format=3]\n[node name=\"Root\" type=\"Node3D\"]\ntransform = Transform3D(1, 2)\n",
		"[gd_scene format=3]\n[node name=\"Root\" type=\"Node3D\"]\n[node name=\"Bad\" parent=\"Missing\"]\n",
		"[gd_scene format=3]\n[sub_resource type=\"ArrayMesh\" id=\"mesh\"]\n_surfaces = [{\"primitive\":3,"
		"\"format\":1,\"vertex_count\":3,\"vertex_data\":PackedByteArray(1, 2)}]\n"
		"[node name=\"Model\" type=\"MeshInstance3D\"]\nmesh = SubResource(\"mesh\")\n",
		"[gd_scene format=3]\n[sub_resource type=\"ArrayMesh\" id=\"mesh\"]\n_surfaces = [{",
		"[gd_scene format=3]\n[node name=\"unterminated]",
		"[gd_scene format=3]\n[node name=\"trailing escape\\",
	};
	for (const char *scene : scenes) {
		const auto archive = io::openMemoryArchive();
		ASSERT_TRUE(add(archive, "bad.tscn", scene));
		GodotSceneFormat format;
		scenegraph::SceneGraph graph;
		EXPECT_FALSE(format.loadGroups("bad.tscn", archive, graph, testLoadCtx)) << scene;
	}
}

} // namespace voxelformat
