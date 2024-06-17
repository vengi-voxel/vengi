/**
 * @file
 */

#include "voxelcollection/CollectionManager.h"
#include "app/tests/AbstractTest.h"
#include "core/SharedPtr.h"
#include "io/Filesystem.h"
#include "video/TexturePool.h"

namespace voxelcollection {

class CollectionManagerTest : public app::AbstractTest {
protected:
	video::TexturePoolPtr _texturePool;
	core::SharedPtr<CollectionManager> _mgr;

	virtual void SetUp() override {
		app::AbstractTest::SetUp();
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

TEST_F(CollectionManagerTest, testOnline) {
	ASSERT_TRUE(_mgr->online(true));
	_mgr->waitOnline();
	_mgr->update(0.0, 1);
	ASSERT_GT(_mgr->allEntries(), 0);
}

} // namespace voxelcollection
