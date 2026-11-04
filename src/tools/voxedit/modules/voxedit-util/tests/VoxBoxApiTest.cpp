/**
 * @file
 */

#include "voxedit-util/VoxBoxApi.h"
#include "app/tests/AbstractTest.h"

namespace voxedit {

class VoxBoxApiTest : public app::AbstractTest {};

TEST_F(VoxBoxApiTest, testAsyncFailures) {
	VoxBoxApi api("http://127.0.0.1:1");
	bool loginDone = false;
	bool searchDone = false;
	api.login("user", "password", [&](bool success) {
		EXPECT_FALSE(success);
		loginDone = true;
	});
	api.search({}, [&](VoxBoxState state) {
		EXPECT_TRUE(state.info.empty());
		searchDone = true;
	});
	for (int i = 0; i < 500 && (!loginDone || !searchDone); ++i) {
		http::update();
		wait(2);
	}
	EXPECT_TRUE(loginDone);
	EXPECT_TRUE(searchDone);
}

TEST_F(VoxBoxApiTest, testLogoutCancelsRequests) {
	if (!http::Request::supported()) {
		GTEST_SKIP();
	}
	VoxBoxApi api("http://127.0.0.1:1");
	bool called = false;
	api.login("user", "password", [&](bool) { called = true; });
	api.logout();
	http::update();
	wait(10);
	http::update();
	EXPECT_FALSE(called);
	EXPECT_FALSE(api.isLoggedIn());
}

TEST_F(VoxBoxApiTest, DISABLED_testSearch) {
	VoxBoxApi api;
	VoxBoxState results;
	bool done = false;
	api.search({}, [&](VoxBoxState state) {
		results = core::move(state);
		done = true;
	});
	for (int i = 0; i < 500 && !done; ++i) {
		http::update();
		wait(10);
	}
	ASSERT_TRUE(done);
	EXPECT_FALSE(results.info.empty());
}

} // namespace voxedit
