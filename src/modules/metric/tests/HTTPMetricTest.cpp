/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "core/Var.h"
#include "core/tests/TestHelper.h"
#include "http/Request.h"
#include "metric/HTTPMetricSender.h"
#include "metric/Metric.h"
#include "util/VarUtil.h"

namespace metric {

class HTTPMetricTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;
};

TEST_F(HTTPMetricTest, testHTTPMetricSenderQueuesAndShutsDown) {
	if (!http::Request::supported()) {
		GTEST_SKIP();
	}
	metric::HTTPMetricSender sender("http://127.0.0.1:1/", "test/1.0");
	ASSERT_TRUE(sender.init());
	EXPECT_TRUE(sender.send("{\"value\":1}"));
	sender.shutdown();
	http::update();
}

// disabled because it requires network access
TEST_F(HTTPMetricTest, DISABLED_testHTTPMetricSender) {
	if (!http::Request::supported()) {
		GTEST_SKIP() << "No http support available";
	}
	util::ScopedVarChange change(cfg::MetricFlavor, "json");
	const core::VarDef metricJsonUrl(cfg::MetricJsonUrl, "https://vengi-voxel.de/api/metric", "", "");
	const core::String &url = core::Var::registerVar(metricJsonUrl)->strVal();
	core::SharedPtr<metric::HTTPMetricSender> sender = core::make_shared<metric::HTTPMetricSender>(url, "test/1.0.0");
	metric::Metric metric;

	ASSERT_TRUE(sender->init()) << "Failed to init metric sender";
	ASSERT_TRUE(metric.init("test", sender));
	EXPECT_TRUE(metric.count("test1", 1));
}

} // namespace metric
