set(SRCS sqlite3.c sqlite3.h)
engine_add_library(
	LIB sqlite3
	SRCS ${SRCS}
	GCCCFLAGS ""
	DEFINES SQLITE_OMIT_LOAD_EXTENSION)
set_target_properties(sqlite3 PROPERTIES LINKER_LANGUAGE C)
