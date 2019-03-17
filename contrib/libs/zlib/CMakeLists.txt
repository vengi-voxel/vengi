set(SRCS
	adler32.c
	compress.c
	crc32.c
	deflate.c
	infback.c
	inffast.c
	inflate.c
	inftrees.c
	trees.c
	uncompr.c
	zutil.c
)

engine_add_library(
	LIB zlib
	SRCS ${SRCS}
	UNITY
)

if (NOT ZLIB_FOUND)
	if (MSVC)
		add_definitions(-D_NO_FSEEKO)
		add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
		add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
	endif()
endif()
