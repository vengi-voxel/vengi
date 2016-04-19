set(SRCS sqlite3.c sqlite3.h)
engine_add_library(
	LIB sqlite3
	SRCS ${SRCS}
	CFLAGS ""
	DEFINES SQLITE_OMIT_LOAD_EXTENSION)
