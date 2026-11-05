/**
 * @file
 */

#include "voxelcollection/CollectionManager.h"
#include "app/tests/AbstractTest.h"
#include "core/SharedPtr.h"
#include "io/Filesystem.h"
#include "video/TexturePool.h"
#include "voxelcollection/Downloader.h"
#include "voxelformat/FormatConfig.h"

namespace voxelcollection {

class CollectionManagerTest : public app::AbstractTest {
protected:
	video::TexturePoolPtr _texturePool;
	core::SharedPtr<CollectionManager> _mgr;

	virtual void SetUp() override {
		app::AbstractTest::SetUp();
		voxelformat::FormatConfig::init();
		_texturePool = core::make_shared<video::TexturePool>();
		// _texturePool->init();
		_mgr = core::make_shared<CollectionManager>(_testApp->filesystem(), _texturePool);
		ASSERT_TRUE(_mgr->init());
		_mgr->setLocalDir(_testApp->filesystem()->homePath());
		ASSERT_EQ(_testApp->filesystem()->homePath(), _mgr->localDir());
	}

	virtual void TearDown() override {
		_texturePool->shutdown();
		_mgr->shutdown();
		_mgr = {};
		app::AbstractTest::TearDown();
	}

	void checkEmptySourceCompletes(const char *json) {
		const char *cacheFile = "github-test-empty-collection-master.json";
		ASSERT_TRUE(_testApp->filesystem()->homeWrite(cacheFile, json));
		VoxelSource source;
		source.name = "Empty collection";
		source.provider = "github";
		source.github.repo = "test/empty-collection";
		source.github.commit = "master";
		_mgr->resolve(source, false);
		ASSERT_TRUE(_mgr->resolved(source));
		_mgr->update(0.0);
		const auto iter = _mgr->voxelFilesMap().find(source.name);
		ASSERT_NE(iter, _mgr->voxelFilesMap().end());
		EXPECT_TRUE(iter->value.files.empty());
		EXPECT_TRUE(iter->value.sorted);
		EXPECT_TRUE(io::Filesystem::sysRemoveFile(_testApp->filesystem()->homeWritePath(cacheFile)));
	}
};

TEST_F(CollectionManagerTest, DISABLED_testLocal) {
	ASSERT_TRUE(_mgr->local());
	_mgr->update(0.0, 1);
	ASSERT_GT(_mgr->allEntries(), 0);
}

TEST_F(CollectionManagerTest, testEmptySourceCompletes) {
	checkEmptySourceCompletes("{\"tree\": []}");
}

TEST_F(CollectionManagerTest, testInvalidSourceResponseCompletes) {
	checkEmptySourceCompletes("{}");
}

TEST_F(CollectionManagerTest, testReloadSource) {
	const char *cacheFile = "github-test-empty-collection-master.json";
	ASSERT_TRUE(_testApp->filesystem()->homeWrite(cacheFile, "{\"tree\": []}"));
	VoxelSource source;
	source.name = "Empty collection";
	source.provider = "github";
	source.github.repo = "test/empty-collection";
	source.github.commit = "master";
	_mgr->resolve(source, false);
	ASSERT_TRUE(_mgr->resolved(source));
	_mgr->update(0.0);
	ASSERT_NE(_mgr->voxelFilesMap().find(source.name), _mgr->voxelFilesMap().end());

	// sync force-reload clears and re-resolves from the (still present) cache
	_mgr->resolve(source, false, true);
	ASSERT_TRUE(_mgr->resolved(source));
	_mgr->update(0.0);
	const auto iter = _mgr->voxelFilesMap().find(source.name);
	ASSERT_NE(iter, _mgr->voxelFilesMap().end());
	EXPECT_TRUE(iter->value.files.empty());
	EXPECT_TRUE(io::Filesystem::sysRemoveFile(_testApp->filesystem()->homeWritePath(cacheFile)));
}

TEST_F(CollectionManagerTest, testCachedAutoResolve) {
	const char *cacheFile = "github-test-empty-collection-master.json";
	ASSERT_TRUE(_testApp->filesystem()->homeWrite(cacheFile, "{\"tree\": []}"));
	VoxelSource source;
	source.name = "Empty collection";
	source.provider = "github";
	source.github.repo = "test/empty-collection";
	source.github.commit = "master";
	ASSERT_TRUE(_mgr->cached(source));

	// inject via the online source queue path used by update()
	_mgr->online(false);
	// directly resolve path covered above; here verify cached() + resolve from cache
	_mgr->resolve(source, false);
	ASSERT_TRUE(_mgr->resolved(source));
	_mgr->update(0.0);
	ASSERT_NE(_mgr->voxelFilesMap().find(source.name), _mgr->voxelFilesMap().end());
	EXPECT_TRUE(io::Filesystem::sysRemoveFile(_testApp->filesystem()->homeWritePath(cacheFile)));
}

TEST_F(CollectionManagerTest, DISABLED_testOnline) {
	const size_t before = _mgr->sources().size();
	ASSERT_TRUE(_mgr->online());
	_mgr->update(0.0, 1);

	ASSERT_GT(_mgr->sources().size(), before);
	bool foundVengi = false;
	for (const auto &source : _mgr->sources()) {
		if (source.name == "Vengi") {
			foundVengi = true;
			_mgr->resolve(source);
			break;
		}
	}
	ASSERT_TRUE(foundVengi) << "Could not find the vengi source";
	_mgr->update(0.0, 10);
	ASSERT_GT(_mgr->allEntries(), 0);

	auto iter = _mgr->voxelFilesMap().find("Vengi");
	ASSERT_TRUE(iter != _mgr->voxelFilesMap().end());
	VoxelFile voxelFile = iter->value.files.front();
	ASSERT_TRUE(_mgr->download(voxelFile));
	ASSERT_TRUE(voxelFile.downloaded);
}

} // namespace voxelcollection
