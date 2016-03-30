#include "core/tests/AbstractTest.h"
#include "backend/storage/Persister.h"

namespace backend {

class PersisterTest : public core::AbstractTest {
};

TEST(PersisterTest, testSaveAndLoadUser) {
	Persister persister;
	ASSERT_TRUE(persister.init()) << "Could not establish database connection";
	persister.initTables();
	// a second call for this will lead to duplicate key constraint errors
	persister.storeUser("test@somedomain.com", "12345", "0");
	const int userId = persister.loadUser("test@somedomain.com", "12345", "0");
	ASSERT_NE(0, userId) << "Could not load user";
}

}
