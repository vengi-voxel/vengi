/**
 * @file
 */

#include "AbstractDatabaseTest.h"
#include "TestModels.h"
#include "persistence/DBHandler.h"
#include "engine-config.h"

namespace persistence {

class DatabaseSchemaUpdateTest: public AbstractDatabaseTest {
private:
	using Super = AbstractDatabaseTest;
protected:
	bool _supported = true;
	persistence::DBHandler _dbHandler;

	bool isDifferent(const db::MetainfoModel& schemaColumn, const Field& field) const {
		if ((uint32_t)schemaColumn.constraintmask() != field.contraintMask) {
			return true;
		}
		if (schemaColumn.columndefault() != field.defaultVal) {
			return true;
		}
		if (toFieldType(schemaColumn.datatype()) != field.type) {
			return true;
		}
		if (schemaColumn.maximumlength() != field.length) {
			return true;
		}
		return false;
	}

	void checkIsCurrent(Model&& model) const {
		Log::info("Check %s", model.tableName());
		std::vector<db::MetainfoModel> schemaModels;
		schemaModels.reserve(model.fields().size() * 2);
		const db::DBConditionMetainfoModelSchemaname c1(model.schema());
		const db::DBConditionMetainfoModelTablename c2(model.tableName());
		const DBConditionMultiple condition(true, { &c1, &c2 });
		ASSERT_TRUE(_dbHandler.select(db::MetainfoModel(), condition, [&] (db::MetainfoModel&& model) {
			schemaModels.emplace_back(model);
		})) << "Failed to execute metainfo select query";
		std::unordered_map<core::String, const db::MetainfoModel*, core::StringHash> map;
		map.reserve(schemaModels.size());
		for (const auto& c : schemaModels) {
			ASSERT_FALSE(c.columnname().empty()) << c.tableName() << " has an invalid entry for the column";
			map.insert(std::make_pair(c.columnname(), &c));
			const Field& f = model.getField(c.columnname());
			ASSERT_FALSE(isDifferent(c, f)) << "Field " << f.name << " differs with db meta info";
		}
	}

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
	checkIsCurrent(db::TestUpdate1Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate2Model()));
	checkIsCurrent(db::TestUpdate2Model());
}

TEST_F(DatabaseSchemaUpdateTest, testRemoveColumns) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate2Model()));
	checkIsCurrent(db::TestUpdate2Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	checkIsCurrent(db::TestUpdate1Model());
}

TEST_F(DatabaseSchemaUpdateTest, testAddAndRemoveMultipleStuffColumns) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	checkIsCurrent(db::TestUpdate1Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate3Model()));
	checkIsCurrent(db::TestUpdate3Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	checkIsCurrent(db::TestUpdate1Model());
}

TEST_F(DatabaseSchemaUpdateTest, testAddAndRemoveSingleStepsColumns) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	checkIsCurrent(db::TestUpdate1Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate2Model()));
	checkIsCurrent(db::TestUpdate2Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate3Model()));
	checkIsCurrent(db::TestUpdate3Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	checkIsCurrent(db::TestUpdate1Model());
}

TEST_F(DatabaseSchemaUpdateTest, testAddAndRemoveSingleStepsReversedColumns) {
	if (!_supported) {
		return;
	}
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	checkIsCurrent(db::TestUpdate1Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate3Model()));
	checkIsCurrent(db::TestUpdate3Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate2Model()));
	checkIsCurrent(db::TestUpdate2Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate1Model()));
	checkIsCurrent(db::TestUpdate1Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate3Model()));
	checkIsCurrent(db::TestUpdate3Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate4Model()));
	checkIsCurrent(db::TestUpdate4Model());
	ASSERT_TRUE(_dbHandler.createOrUpdateTable(db::TestUpdate5Model()));
	checkIsCurrent(db::TestUpdate5Model());
}

}
