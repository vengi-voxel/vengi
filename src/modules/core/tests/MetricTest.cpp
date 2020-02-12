/**
 * @file
 */

#include "core/tests/AbstractTest.h"
#include "core/metric/Metric.h"
#include "core/metric/IMetricSender.h"
#include "core/Var.h"

namespace metric {

class BufferSender : public IMetricSender {
private:
	mutable core::String _lastBuffer;
public:

	bool send(const char* buffer) const override {
		_lastBuffer = buffer;
		return true;
	}

	inline const core::String& metricLine() const {
		return _lastBuffer;
	}
};

#define PREFIX "test"

class MetricTest: public core::AbstractTest {
private:
	using Super = core::AbstractTest;
protected:
	std::shared_ptr<BufferSender> sender;
	void SetUp() override {
		Super::SetUp();
		sender = std::make_shared<BufferSender>();
		ASSERT_TRUE(sender->init());
	}

	void TearDown() override {
		Super::TearDown();
		sender->shutdown();
	}

	inline core::String count(const char *id, int value, Flavor flavor, const TagMap& tags = {}) const {
		setFlavor(flavor);
		Metric m;
		m.init(PREFIX, sender);
		m.count(id, value, tags);
		return sender->metricLine();
	}

	inline core::String gauge(const char *id, int value, Flavor flavor, const TagMap& tags = {}) const {
		setFlavor(flavor);
		Metric m;
		m.init(PREFIX, sender);
		m.gauge(id, value, tags);
		return sender->metricLine();
	}

	inline core::String timing(const char *id, int value, Flavor flavor, const TagMap& tags = {}) const {
		setFlavor(flavor);
		Metric m;
		m.init(PREFIX, sender);
		m.timing(id, value, tags);
		return sender->metricLine();
	}

	inline void setFlavor(Flavor flavor) const {
		if (flavor == Flavor::Telegraf) {
			core::Var::get("metric_flavor", "")->setVal("telegraf");
		} else if (flavor == Flavor::Etsy) {
			core::Var::get("metric_flavor", "")->setVal("etsy");
		} else if (flavor == Flavor::Datadog) {
			core::Var::get("metric_flavor", "")->setVal("datadog");
		} else if (flavor == Flavor::Influx) {
			core::Var::get("metric_flavor", "")->setVal("influx");
		}
	}
};

TEST_F(MetricTest, testCounterIncreaseOne) {
	EXPECT_EQ(count("test1", 1, Flavor::Etsy), PREFIX ".test1:1|c");
}

TEST_F(MetricTest, testCounterIncreaseTwo) {
	EXPECT_EQ(count("test2", 2, Flavor::Etsy), PREFIX ".test2:2|c");
}

TEST_F(MetricTest, testGaugeValueOne) {
	EXPECT_EQ(gauge("test1", 1, Flavor::Etsy), PREFIX ".test1:1|g");
}

TEST_F(MetricTest, testGaugeValueTwo) {
	EXPECT_EQ(gauge("test2", 2, Flavor::Etsy), PREFIX ".test2:2|g");
}

TEST_F(MetricTest, testTimingValueOne) {
	EXPECT_EQ(timing("test1", 1, Flavor::Etsy), PREFIX ".test1:1|ms");
}

TEST_F(MetricTest, testTimingValueTwo) {
	EXPECT_EQ(timing("test2", 2, Flavor::Etsy), PREFIX ".test2:2|ms");
}

TEST_F(MetricTest, testTimingSingleTag) {
	const TagMap map {{"key1", "value1"}};
	EXPECT_EQ(timing("test", 1, Flavor::Etsy, map), PREFIX ".test:1|ms")
		<< "Expected to get no tags on etsy flavor";
	EXPECT_EQ(timing("test", 1, Flavor::Telegraf, map), PREFIX ".test,key1=value1:1|ms")
		<< "Expected to get tags after key in telegraf flavor";
	EXPECT_EQ(timing("test", 1, Flavor::Datadog, map), PREFIX ".test:1|ms|#key1:value1")
		<< "Expected to get tags after type in datadog flavor";
	EXPECT_EQ(timing("testkey", 1, Flavor::Influx, map), PREFIX "_testkey,type=ms,key1=value1 value=1")
		<< "Unexpected influx format";
}

// The order is not stable - thus the result string order of the tag can differ
TEST_F(MetricTest, DISABLED_testTimingMultipleTags) {
	const TagMap map {{"key1", "value1"}, {"key2", "value2"}};
	EXPECT_EQ(timing("test", 1, Flavor::Etsy, map), PREFIX ".test:1|ms")
		<< "Expected to get no tags on etsy flavor";
	EXPECT_EQ(timing("test", 1, Flavor::Telegraf, map), PREFIX ".test,key1=value1,key2=value2:1|ms")
		<< "Expected to get tags after key in telegraf flavor";
	EXPECT_EQ(timing("test", 1, Flavor::Datadog, map), PREFIX ".test:1|ms|#key1:value1,key2:value2")
		<< "Expected to get tags after type in datadog flavor";
}

}
