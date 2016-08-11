set(SRCS
	TestApp.cpp TestApp.h
	TestMeshApp.cpp TestMeshApp.h
)
set(LIB testcore)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core frontend video)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
