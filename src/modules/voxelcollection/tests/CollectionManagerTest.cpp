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
		ASSERT_TRUE(_mgr->setLocalDir(_testApp->filesystem()->homePath()));
	}

	virtual void TearDown() override {
		_texturePool->shutdown();
		_mgr->shutdown();
		_mgr = {};
		app::AbstractTest::TearDown();
	}
};

TEST_F(CollectionManagerTest, testLocal) {
	ASSERT_TRUE(_mgr->local());
	_mgr->waitLocal();
	_mgr->update(0.0, 1);
	ASSERT_GT(_mgr->allEntries(), 0);
}

TEST_F(CollectionManagerTest, DISABLED_testOnline) {
	const size_t before = _mgr->sources().size();
	ASSERT_TRUE(_mgr->online(true));
	_mgr->waitOnline();
	_mgr->update(0.0, 1);

	ASSERT_GT(_mgr->sources().size(), before);
	bool foundVengi = false;
	for (const auto &source : _mgr->sources()) {
		if (source.name == "Vengi") {
			foundVengi = true;
			_mgr->resolve(source, false);
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
