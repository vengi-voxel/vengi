# Level 3 (frame) build - see https://github.com/lz4/lz4/blob/dev/lib/README.md
# Prefer system liblz4 via Findlz4.cmake.
set(SRCS
	lz4.c
	lz4hc.c
	lz4frame.c
	xxhash.c
)

set(LIB lz4)
engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
)
