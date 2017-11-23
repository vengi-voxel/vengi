/**
 * @file
 */

#include "AbstractDatabaseTest.h"
#include "TestModels.h"
#include "persistence/ConnectionPool.h"
#include "persistence/DBHandler.h"
#include "engine-config.h"

namespace persistence {

class DatabaseSchemaUpdateTest: public AbstractDatabaseTest {
private:
	using Super = AbstractDatabaseTest;
protected:
	bool _supported = true;
	persistence::DBHandler _dbHandler;
public:
	void SetUp() override {
		Super::SetUp();
		_supported = _dbHandler.init();
		if (_supported) {
			_dbHandler.dropTable(db::TestUpdate1Model());
		} else {
			Log::warn("DatabaseSchemaUpdateTest is skipped");
		}
	}

	void TearDown() override {
		Super::TearDown();
		_dbHandler.shutdown();
	}
};

TEST_F(DatabaseSchemaUpdateTest, testAddNewColumns) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate2Model()));
}

TEST_F(DatabaseSchemaUpdateTest, testRemoveColumns) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate2Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
}

TEST_F(DatabaseSchemaUpdateTest, testAddAndRemoveMultipleStuffColumns) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate3Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
}

TEST_F(DatabaseSchemaUpdateTest, testAddAndRemoveSingleStepsColumns) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate2Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate3Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
}

TEST_F(DatabaseSchemaUpdateTest, testAddAndRemoveSingleStepsReversedColumns) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate3Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate2Model()));
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
}

}
