set(SRCS
	MailSender.h MailSender.cpp
)
set(LIB mail)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core libcurl)

gtest_suite_sources(tests tests/MailSenderTest.cpp)
gtest_suite_deps(tests ${LIB})
