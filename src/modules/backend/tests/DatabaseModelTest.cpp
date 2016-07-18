/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "DatabaseModels.h"
#include "config.h"

namespace backend {

class DatabaseModelTest: public core::AbstractTest {
};

TEST_F(DatabaseModelTest, testCreate) {
	persistence::UserStore u;
	ASSERT_TRUE(u.createTable()) << "Could not create table";
}

TEST_F(DatabaseModelTest, testWrite) {
	const std::string email = "a@b.c";
	const std::string password = "secret";
	const ::persistence::Timestamp ts = ::persistence::Timestamp::now();
	persistence::UserStore u(&email, &password, &ts);
	ASSERT_NE(0, u.userid());
}

}
