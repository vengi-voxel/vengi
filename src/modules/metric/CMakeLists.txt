set(SRCS
	Metric.h Metric.cpp
	UDPMetricSender.h UDPMetricSender.cpp
	IMetricSender.h
	MetricEvent.h
)
set(LIB metric)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/MetricTest.cpp
)
gtest_suite_deps(tests ${LIB})
