set(SRCS
	MailSender.h MailSender.cpp
)
set(LIB mail)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core libcurl)
target_compile_definitions(${LIB} PUBLIC "-DCURL_STATICLIB")

set(TEST_SRCS
	tests/MailSenderTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
