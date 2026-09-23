/**
 * @file
 */

#include "voxedit-util/VoxBoxApi.h"
#include "app/tests/AbstractTest.h"
#include "http/Request.h"
#include "http/RequestAsync.h"
#include "core/concurrent/Atomic.h"

namespace voxedit {

class VoxBoxApiTest : public app::AbstractTest {};

TEST_F(VoxBoxApiTest, DISABLED_testSearch) {
	if (!http::Request::supported()) {
		GTEST_SKIP() << "No http support available";
	}

	VoxBoxApi api;
	core::AtomicBool called{false};
	VoxBoxState results;
	api.searchAsync({}, [&](VoxBoxState state) {
		results = core::move(state);
		called = true;
	});

	bool finished = false;
	for (int i = 0; i < 500 && !finished; ++i) {
		http::update();
		if (called) {
			finished = true;
			break;
		}
		_testApp->wait(10);
	}

	ASSERT_TRUE(finished) << "async voxbox search callback was not dispatched";
	EXPECT_FALSE(results.info.empty());
	http::clearPending();
}

} // namespace voxedit
