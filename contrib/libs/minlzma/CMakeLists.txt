set(SRCS
	dictbuf.c
	inputbuf.c
	lzma2dec.c
	lzmadec.c
	rangedec.c
	xzstream.c
	minlzlib.h
	lzma2dec.h
	lzmadec.h
	xzstream.h
)
add_library(minlzma STATIC ${SRCS})
target_include_directories(minlzma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
engine_target_optimize(minlzma)
