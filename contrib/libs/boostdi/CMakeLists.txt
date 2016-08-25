set(SRCS
	dummy.cpp
	boost/di.hpp
)

engine_add_library(
	LIB boostdi
	SRCS ${SRCS}
)
