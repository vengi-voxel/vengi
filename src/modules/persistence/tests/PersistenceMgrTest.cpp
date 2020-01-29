/**
 * @file
 */

#include "AbstractDatabaseTest.h"
#include "persistence/PersistenceMgr.h"
#include "TestModels.h"

namespace persistence {

class PersistenceMgrTest : public AbstractDatabaseTest, public persistence::ISavable {
private:
	using Super = AbstractDatabaseTest;
protected:
	bool _supported = true;
	persistence::DBHandlerPtr _dbHandler;
	Models _dirtyModels;
	int _executeStateUpdate = 0;
public:
	void SetUp() override {
		Super::SetUp();
		_dbHandler = std::make_shared<persistence::DBHandler>();
		_supported = _dbHandler->init();
		if (_supported) {
			_dbHandler->createOrUpdateTable(db::TestModel());
			_dbHandler->truncate(db::TestModel());
			Log::debug("PersistenceMgrTest: Finished setup");
		} else {
			Log::warn("PersistenceMgrTest is skipped");
		}
		_executeStateUpdate = 0;
	}

	void TearDown() override {
		Super::TearDown();
		_dbHandler->shutdown();
	}

	bool getDirtyModels(Models& models) override {
		_executeStateUpdate++;
		if (_dirtyModels.empty()) {
			return false;
		}
		std::copy(_dirtyModels.begin(), _dirtyModels.end(), std::back_inserter(models));
		_dirtyModels.clear();
		return true;
	}

	void update(PersistenceMgr& mgr, const db::TestModel& in, db::TestModel* out = nullptr) {
		EXPECT_TRUE(mgr.init());
		EXPECT_TRUE(mgr.registerSavable(FourCC('F','O','O','O'), this));
		_dirtyModels.push_back(&in);
		mgr.update(0l);
		EXPECT_TRUE(_dirtyModels.empty());
		EXPECT_TRUE(mgr.unregisterSavable(FourCC('F','O','O','O'), this));
		mgr.shutdown();
		if (in.shouldBeDeleted()) {
			return;
		}
		int found = 0;
		EXPECT_TRUE(_dbHandler->select(db::TestModel(), DBConditionOne(), [&] (db::TestModel&& mdl) {
			++found;
			if (out) {
				*out = mdl;
			}
		}));
		EXPECT_GE(found, 1) << "Failed to find the inserted entry";
	}

	void relativeUpdate(PersistenceMgr& mgr, db::TestModel mdl, int initial, int delta) {
		mdl.setPoints(initial);
		db::TestModel out;
		update(mgr, mdl, &out);
		ASSERT_NE(out.points(), nullptr);
		EXPECT_EQ(*out.points(), initial);
		mdl.setPoints(delta);
		update(mgr, mdl, &out);
		ASSERT_NE(out.points(), nullptr);
		EXPECT_EQ(*out.points(), initial + delta);
	}

	db::TestModel create(int64_t id = 1, const core::String& prefix = "", const core::String& email = "foo@b.ar",
			const core::String& name = "foobar", const core::String& password = "secret") const {
		db::TestModel mdl;
		mdl.setId(id);
		mdl.setEmail(prefix + email);
		mdl.setName(prefix + name);
		mdl.setPassword(password);
		return mdl;
	}
};

TEST_F(PersistenceMgrTest, testSavable) {
	if (!_supported) {
		return;
	}
	PersistenceMgr mgr(_dbHandler);
	EXPECT_TRUE(mgr.init());
	EXPECT_TRUE(mgr.registerSavable(FourCC('F','O','O','O'), this));
	mgr.update(0l);
	EXPECT_EQ(_executeStateUpdate, 1);
	mgr.update(0l);
	EXPECT_EQ(_executeStateUpdate, 2);
	EXPECT_TRUE(mgr.unregisterSavable(FourCC('F','O','O','O'), this));
	mgr.shutdown();
}

TEST_F(PersistenceMgrTest, testSavableUpdate) {
	if (!_supported) {
		return;
	}
	PersistenceMgr mgr(_dbHandler);
	update(mgr, create());
}

TEST_F(PersistenceMgrTest, testSavableDelete) {
	if (!_supported) {
		return;
	}
	PersistenceMgr mgr(_dbHandler);
	db::TestModel mdl = create();
	mdl.flagForDelete();
	update(mgr, mdl);
}

TEST_F(PersistenceMgrTest, testSavableRelativeUpdate) {
	if (!_supported) {
		return;
	}
	PersistenceMgr mgr(_dbHandler);
	relativeUpdate(mgr, create(), 1, 1);
}

TEST_F(PersistenceMgrTest, testSavableRelativeUpdateNegative) {
	if (!_supported) {
		return;
	}
	PersistenceMgr mgr(_dbHandler);
	relativeUpdate(mgr, create(), 100, -110);
}

}
