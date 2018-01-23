/**
 * @file
 */

#include "AbstractDatabaseTest.h"
#include "persistence/PersistenceMgr.h"

namespace persistence {

class PersistenceMgrTest : public AbstractDatabaseTest, public persistence::ISavable {
private:
	using Super = AbstractDatabaseTest;
protected:
	bool _supported = true;
	persistence::DBHandlerPtr _dbHandler;
public:
	void SetUp() override {
		Super::SetUp();
		_dbHandler = std::make_shared<persistence::DBHandler>();
		_supported = _dbHandler->init();
		if (_supported) {
			Log::debug("PersistenceMgrTest: Finished setup");
		} else {
			Log::warn("PersistenceMgrTest is skipped");
		}
	}

	void TearDown() override {
		Super::TearDown();
		_dbHandler->shutdown();
	}

	bool getDirtyModels(Models& models) override {
		return false;
	}
};

TEST_F(PersistenceMgrTest, testSavable) {
	if (!_supported) {
		return;
	}
	PersistenceMgr mgr(_dbHandler);
	ASSERT_TRUE(mgr.init());
	mgr.registerSavable(FourCC('F','O','O','O'), this);
	mgr.update(0l);
	mgr.unregisterSavable(FourCC('F','O','O','O'), this);
	mgr.shutdown();
}

}
