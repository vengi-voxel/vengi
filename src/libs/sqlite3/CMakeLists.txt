fips_begin_module(sqlite3)
	fips_files(
		sqlite3.c
		sqlite3.h
	)
fips_end_module()
add_definitions(-DSQLITE_OMIT_LOAD_EXTENSION)
target_include_directories(sqlite3 PUBLIC .)
