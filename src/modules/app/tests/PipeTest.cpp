/**
 * @file
 */

#include "app/Pipe.h"
#include "app/tests/AbstractTest.h"
#include "core/ConfigVar.h"
#include "util/VarUtil.h"
#include <gtest/gtest.h>

namespace app {

class PipeTest : public app::AbstractTest {};

TEST_F(PipeTest, testInitDataShutdown) {
	Pipe pipe;
	pipe.construct();
	util::ScopedVarChange scoped(cfg::AppPipe, "true");
	ASSERT_TRUE(pipe.init());
	pipe.shutdown();
}

TEST_F(PipeTest, testInitDisabled) {
	Pipe pipe;
	pipe.construct();
	util::ScopedVarChange scoped(cfg::AppPipe, "false");
	ASSERT_TRUE(pipe.init());
	pipe.shutdown();
}

} // namespace app
