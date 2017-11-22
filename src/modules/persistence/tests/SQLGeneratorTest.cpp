/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "persistence/SQLGenerator.h"
#include "TestModels.h"

namespace persistence {

class SQLGeneratorTest : public core::AbstractTest {
};

TEST_F(SQLGeneratorTest, testDelete) {
	ASSERT_EQ(R"(DELETE FROM "public"."test")", createDeleteStatement(db::TestModel()));
}

TEST_F(SQLGeneratorTest, testDeleteWithPk) {
	db::TestModel model;
	model.setId(1L);
	ASSERT_EQ(R"(DELETE FROM "public"."test" WHERE "id" = $1)", createDeleteStatement(model));
}

TEST_F(SQLGeneratorTest, testDrop) {
	ASSERT_EQ(R"(DROP TABLE IF EXISTS "public"."test";DROP SEQUENCE IF EXISTS public.test_id_seq;)",
			createDropTableStatement(db::TestModel()));
}

TEST_F(SQLGeneratorTest, testTruncate) {
	ASSERT_EQ(R"(TRUNCATE TABLE "public"."test";)", createTruncateTableStatement(db::TestModel()));
}

TEST_F(SQLGeneratorTest, testCreateWithoutMeta) {
	ASSERT_EQ(R"(CREATE TABLE IF NOT EXISTS "public"."testupdate" ("id" BIGINT PRIMARY KEY);)",
			createCreateTableStatement(db::TestUpdate1Model(), false));
}

TEST_F(SQLGeneratorTest, testUpdateWithPk) {
	db::TestModel model;
	model.setId(1L);
	model.setName("testname");
	ASSERT_EQ(R"(UPDATE "public"."test" SET "name" = $1 WHERE "id" = $2)", createUpdateStatement(model));
}

TEST_F(SQLGeneratorTest, testUpdateWithoutPk) {
	db::TestModel model;
	model.setName("testname");
	ASSERT_EQ(R"(UPDATE "public"."test" SET "name" = $1)", createUpdateStatement(model));
}

TEST_F(SQLGeneratorTest, testRelativeUpdateViaInsert) {
	db::TestModel model;
	model.setId(1L);
	model.setPoints(42L);
	ASSERT_EQ(R"(INSERT INTO "public"."test" ("id", "points") VALUES ($1, $2) ON CONFLICT ("id") DO UPDATE SET "points" = "public"."test"."points" + $3 RETURNING "id";)",
			createInsertStatement(model));
}

TEST_F(SQLGeneratorTest, testInsert) {
	db::TestModel model;
	model.setName("testname");
	ASSERT_EQ(R"(INSERT INTO "public"."test" ("name") VALUES ($1) RETURNING "id";)",
			createInsertStatement(model));
}

TEST_F(SQLGeneratorTest, testInsertAutoIncrementGiven) {
	db::TestModel model;
	model.setName("testname");
	model.setId(1L);
	ASSERT_EQ(R"(INSERT INTO "public"."test" ("id", "name") VALUES ($1, $2) ON CONFLICT ("id") DO UPDATE SET "name" = $3 RETURNING "id";)",
			createInsertStatement(model));
}

}
