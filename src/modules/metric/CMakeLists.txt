set(SRCS
	Metric.h Metric.cpp
	UDPMetricSender.h UDPMetricSender.cpp
	IMetricSender.h
	MetricEvent.h
)
set(LIB metric)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)

gtest_suite_sources(tests
	tests/MetricTest.cpp
)
gtest_suite_deps(tests ${LIB})
