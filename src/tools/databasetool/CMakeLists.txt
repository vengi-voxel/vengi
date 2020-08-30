project(databasetool)
set(SRCS
	DatabaseTool.h DatabaseTool.cpp
	Util.h Util.cpp
	Generator.h Generator.cpp
	Parser.h Parser.cpp
	Mapping.h
	Table.h
	../../modules/persistence/FieldType.cpp
)
engine_add_build_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} NOINSTALL)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES app)
