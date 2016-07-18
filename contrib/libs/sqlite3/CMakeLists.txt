engine_add_library(
	LIB sqlite3
	SRCS sqlite3.c
	GCCCFLAGS "-Wno-cast-qual"
	DEFINES SQLITE_HAVE_ISNAN SQLITE_THREADSAFE=0)
