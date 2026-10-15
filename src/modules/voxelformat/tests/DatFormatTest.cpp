/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "core/collection/Buffer.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "io/ZipWriteStream.h"
#include "util/VarUtil.h"
#include "voxelformat/private/minecraft/DatFormat.h"
#include "voxelformat/private/minecraft/NamedBinaryTag.h"

namespace voxelformat {

class DatFormatTest : public AbstractFormatTest {
protected:
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

	io::MemoryArchivePtr createWorldArchive(int32_t spawnX, int32_t spawnY, int32_t spawnZ) {
		io::ArchivePtr fs = helper_filesystemarchive();
		io::MemoryArchivePtr archive = io::openMemoryArchive();
		// Same MCA content under two region names - DatFormat filters by filename coords.
		if (!addArchiveFile(fs, "r.0.-2.mca", archive, "region/r.0.-2.mca")) {
			return {};
		}
		if (!addArchiveFile(fs, "r.0.-2.mca", archive, "region/r.5.5.mca")) {
			return {};
		}

		priv::NBTCompound data;
		data.put("SpawnX", priv::NamedBinaryTag(spawnX));
		data.put("SpawnY", priv::NamedBinaryTag(spawnY));
		data.put("SpawnZ", priv::NamedBinaryTag(spawnZ));
		data.put("LevelName", priv::NamedBinaryTag(core::String("DatFormatTest")));
		priv::NBTCompound root;
		root.put("Data", priv::NamedBinaryTag(core::move(data)));

		io::BufferedReadWriteStream levelStream;
		{
			io::ZipWriteStream zipStream(levelStream);
			if (!priv::NamedBinaryTag::write(priv::NamedBinaryTag(core::move(root)), "", zipStream)) {
				return {};
			}
		}
		if (!archive->add("level.dat", (const uint8_t *)levelStream.getBuffer(), (size_t)levelStream.size())) {
			return {};
		}
		return archive;
	}
};

TEST_F(DatFormatTest, testLoadMergeRegions) {
	const io::MemoryArchivePtr archive = createWorldArchive(0, 64, -800);
	ASSERT_TRUE(archive);
	util::ScopedVarChange radius(cfg::VoxformatMCLoadRadius, "0");
	util::ScopedVarChange merge(cfg::VoxformatMCMergeRegions, "true");

	DatFormat format;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format.load("level.dat", archive, sceneGraph, testLoadCtx));
	EXPECT_EQ(1u, sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
}

TEST_F(DatFormatTest, testLoadNoMergeRegions) {
	const io::MemoryArchivePtr archive = createWorldArchive(0, 64, -800);
	ASSERT_TRUE(archive);
	util::ScopedVarChange radius(cfg::VoxformatMCLoadRadius, "0");
	util::ScopedVarChange merge(cfg::VoxformatMCMergeRegions, "false");

	DatFormat format;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format.load("level.dat", archive, sceneGraph, testLoadCtx));
	EXPECT_EQ(128u, sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
}

TEST_F(DatFormatTest, testLoadRadiusFiltersRegions) {
	const io::MemoryArchivePtr archive = createWorldArchive(0, 64, -800);
	ASSERT_TRUE(archive);
	util::ScopedVarChange radius(cfg::VoxformatMCLoadRadius, "0");
	util::ScopedVarChange merge(cfg::VoxformatMCMergeRegions, "true");

	DatFormat format;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format.load("level.dat", archive, sceneGraph, testLoadCtx));
	// spawn region r.0.-2 only; far region r.5.5 must be skipped
	EXPECT_EQ(1u, sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
}

TEST_F(DatFormatTest, testLoadRegionAABBFiltersRegions) {
	const io::MemoryArchivePtr archive = createWorldArchive(0, 64, -800);
	ASSERT_TRUE(archive);
	util::ScopedVarChange radius(cfg::VoxformatMCLoadRadius, "-1");
	util::ScopedVarChange region(cfg::VoxformatMCLoadRegion, "0 -64 -1024 511 320 -513");
	util::ScopedVarChange merge(cfg::VoxformatMCMergeRegions, "true");

	DatFormat format;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format.load("level.dat", archive, sceneGraph, testLoadCtx));
	EXPECT_EQ(1u, sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
}

TEST_F(DatFormatTest, testLoadRadiusRejectsEmptyFilter) {
	const io::MemoryArchivePtr archive = createWorldArchive(2560, 64, 2560);
	ASSERT_TRUE(archive);
	// spawn in r.5.5; radius 0 excludes r.0.-2 and keeps only the far region
	util::ScopedVarChange radius(cfg::VoxformatMCLoadRadius, "0");
	util::ScopedVarChange merge(cfg::VoxformatMCMergeRegions, "true");

	DatFormat format;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(format.load("level.dat", archive, sceneGraph, testLoadCtx));
	EXPECT_EQ(1u, sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels));
}

} // namespace voxelformat
