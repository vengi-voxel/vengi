set(SRCS
	Metric.h Metric.cpp
	MetricFacade.h MetricFacade.cpp

	HTTPMetricSender.h HTTPMetricSender.cpp
	UDPMetricSender.h UDPMetricSender.cpp
	IMetricSender.h
)

set(LIB metric)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core http)

set(TEST_SRCS
	tests/MetricTest.cpp
	tests/HTTPMetricTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_deps(tests-${LIB} ${LIB} test-app)
gtest_suite_end(tests-${LIB})
